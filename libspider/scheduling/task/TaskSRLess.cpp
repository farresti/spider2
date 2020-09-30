/*
 * Copyright or © or Copr. IETR/INSA - Rennes (2020) :
 *
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2020)
 *
 * Spider is a dataflow based runtime used to execute dynamic PiSDF
 * applications. The Preesm tool may be used to design PiSDF applications.
 *
 * This software is governed by the CeCILL  license under French law and
 * abiding by the rules of distribution of free software.  You can  use,
 * modify and/ or redistribute the software under the terms of the CeCILL
 * license as circulated by CEA, CNRS and INRIA at the following URL
 * "http://www.cecill.info".
 *
 * As a counterpart to the access to the source code and  rights to copy,
 * modify and redistribute granted by the license, users are provided only
 * with a limited warranty  and the software's author,  the holder of the
 * economic rights,  and the successive licensors  have only  limited
 * liability.
 *
 * In this respect, the user's attention is drawn to the risks associated
 * with loading,  using,  modifying and/or developing or reproducing the
 * software by the user in light of its specific status of free software,
 * that may mean  that it is complicated to manipulate,  and  that  also
 * therefore means  that it is reserved for developers  and  experienced
 * professionals having in-depth computer knowledge. Users are therefore
 * encouraged to load and test the software's suitability as regards their
 * requirements in conditions enabling the security of their systems and/or
 * data to be ensured and,  more generally, to use and operate it in the
 * same conditions as regards security.
 *
 * The fact that you are presently reading this means that you have had
 * knowledge of the CeCILL license and that you accept its terms.
 */

/* === Include(s) === */

#include <scheduling/task/TaskSRLess.h>
#include <scheduling/schedule/Schedule.h>
#include <graphs-tools/helper/pisdf-helper.h>
#include <graphs-tools/transformation/srless/GraphHandler.h>
#include <graphs-tools/transformation/srless/FiringHandler.h>
#include <graphs/pisdf/Vertex.h>
#include <graphs/pisdf/ExternInterface.h>
#include <graphs/pisdf/Edge.h>
#include <graphs/pisdf/Graph.h>
#include <runtime/special-kernels/specialKernels.h>

/* === Static function === */

namespace spider {

    static u32 countDependencies(const srless::ExecDependency &dep) {
        u32 count = 0;
        if (dep.first_.vertex_) {
            count += dep.first_.firingEnd_ - dep.first_.firingStart_ + 1u;
        }
        if (dep.second_.vertex_) {
            count += dep.second_.firingEnd_ - dep.second_.firingStart_ + 1u;
        }
        return count;
    }

    static u32 countDependencies(const spider::vector<srless::ExecDependency> &dependencies) {
        u32 count = 0;
        for (auto &dep : dependencies) {
            count += countDependencies(dep);
        }
        return count;
    }
}

/* === Method(s) implementation === */

spider::sched::TaskSRLess::TaskSRLess(srless::FiringHandler *handler, const pisdf::Vertex *vertex, u32 firing) :
        Task(),
        handler_{ handler },
        vertex_{ vertex },
        firing_{ firing },
        dependenciesCount_{ 0u } {
    size_t mergedFifoCount{ 0u };
    for (const auto *edge : vertex->inputEdgeVector()) {
        const auto edgeIx = static_cast<u32>(edge->sinkPortIx());
        const auto current = dependenciesCount_;
        if (edge->source()->hierarchical() || edge->source()->subtype() == pisdf::VertexType::INPUT) {
            dependenciesCount_ += countDependencies(handler_->computeRelaxedExecDependency(vertex_, firing_, edgeIx));
        } else {
            dependenciesCount_ += countDependencies(handler_->computeExecDependency(vertex_, firing_, edgeIx));
        }
        mergedFifoCount += ((current + 1) < dependenciesCount_);
    }
    if (vertex_->subtype() == pisdf::VertexType::INPUT) {
        const auto *graph = vertex_->graph();
        const auto graphFiring = handler_->firingValue();
        const auto *graphHandler = handler_->getChildFiring(graph, graphFiring);
        const auto dependencies = graphHandler->computeRelaxedExecDependency(graph, graphFiring,
                                                                             static_cast<u32>(vertex_->ix()));
        dependenciesCount_ = countDependencies(dependencies);
    }
    fifos_ = spider::make_shared<AllocatedFifos, StackID::SCHEDULE>(dependenciesCount_ + mergedFifoCount,
                                                                    vertex->outputEdgeCount());
    execInfo_.dependencies_ = spider::make_unique(allocate<Task *, StackID::SCHEDULE>(dependenciesCount_));
    auto *beginIt = execInfo_.dependencies_.get();
    std::fill(beginIt, std::next(beginIt, static_cast<long>(dependenciesCount_)), nullptr);
}

spider::sched::Task *spider::sched::TaskSRLess::previousTask(size_t ix) const {
#ifndef NDEBUG
    if (ix >= dependenciesCount_) {
        throwSpiderException("index out of bound.");
    }
#endif
    return execInfo_.dependencies_.get()[ix];
}

void spider::sched::TaskSRLess::updateTaskExecutionDependencies(const Schedule *schedule) {
    size_t i = 0u;
    if (vertex_->subtype() == pisdf::VertexType::INPUT) {
        const auto *graph = vertex_->graph();
        const auto graphFiring = handler_->firingValue();
        const auto *graphHandler = handler_->getChildFiring(graph, graphFiring);
        const auto edgeIx = static_cast<u32>(vertex_->ix());
        const auto dependencies = graphHandler->computeRelaxedExecDependency(graph, graphFiring, edgeIx);
        for (auto &dep : dependencies) {
            i = updateTaskExecutionDependency(schedule, dep, i);
        }
    } else {
        for (const auto *edge : vertex_->inputEdgeVector()) {
            const auto edgeIx = static_cast<u32>(edge->sinkPortIx());
            if (edge->source()->hierarchical() || edge->source()->subtype() == pisdf::VertexType::INPUT) {
                const auto dependencies = handler_->computeRelaxedExecDependency(vertex_, firing_, edgeIx);
                for (auto &dep : dependencies) {
                    i = updateTaskExecutionDependency(schedule, dep, i);
                }
            } else {
                const auto dep = handler_->computeExecDependency(vertex_, firing_, edgeIx);
                i = updateTaskExecutionDependency(schedule, dep, i);
            }
        }
    }
}

void spider::sched::TaskSRLess::updateExecutionConstraints() {
    auto *execDependencies = execInfo_.dependencies_.get();
    auto *execConstraints = execInfo_.constraints_.get();
    const auto lrtCount = archi::platform()->LRTCount();
    std::fill(execConstraints, execConstraints + lrtCount, SIZE_MAX);
    auto shouldNotifyArray = array<size_t>(lrtCount, SIZE_MAX, StackID::SCHEDULE);
    for (u32 i = 0; i < dependenciesCount_; ++i) {
        auto *dependency = execDependencies[i];
        if (dependency) {
            const auto *depLRT = dependency->mappedLRT();
            const auto currentJobConstraint = execConstraints[depLRT->virtualIx()];
            if ((currentJobConstraint == SIZE_MAX) || (dependency->jobExecIx() > currentJobConstraint)) {
                execConstraints[depLRT->virtualIx()] = dependency->jobExecIx();
                shouldNotifyArray[depLRT->virtualIx()] = i;
            }
        }
    }
    for (const auto &value : shouldNotifyArray) {
        if (value != SIZE_MAX) {
            execDependencies[value]->setNotificationFlag(mappedLRT()->virtualIx(), true);
        }
    }
}

void spider::sched::TaskSRLess::setExecutionDependency(size_t ix, spider::sched::Task *task) {
#ifndef NDEBUG
    if (ix >= dependenciesCount_) {
        throwSpiderException("index out of bound.");
    }
#endif
    if (task) {
        execInfo_.dependencies_.get()[ix] = task;
    }
}

spider::sched::AllocationRule spider::sched::TaskSRLess::allocationRuleForInputFifo(size_t ix) const {
#ifndef NDEBUG
    if (ix >= vertex_->inputEdgeCount()) {
        throwSpiderException("index out of bound.");
    }
#endif
    if (vertex_->subtype() == pisdf::VertexType::INPUT) {
        const auto *graph = vertex_->graph();
        const auto graphFiring = handler_->firingValue();
        const auto *graphHandler = handler_->getChildFiring(graph, graphFiring);
        const auto edgeIx = static_cast<u32>(vertex_->ix());
        const auto dependencies = graphHandler->computeRelaxedExecDependency(graph, graphFiring, edgeIx);
        return allocateInputFifo(dependencies, graph->inputEdge(edgeIx));
    } else {
        const auto *edge = vertex_->inputEdge(ix);
        if (edge->source()->hierarchical() || edge->source()->subtype() == pisdf::VertexType::INPUT) {
            const auto dependencies = handler_->computeRelaxedExecDependency(vertex_, firing_, static_cast<u32>(ix));
            return allocateInputFifo(dependencies, edge);
        } else {
            const auto dep = handler_->computeExecDependency(vertex_, firing_, static_cast<u32>(ix));
            return allocateInputFifo(dep, edge);
        }
    }
}

spider::sched::AllocationRule spider::sched::TaskSRLess::allocationRuleForOutputFifo(size_t ix) const {
#ifndef NDEBUG
    if (ix >= vertex_->outputEdgeCount()) {
        throwSpiderException("index out of bound.");
    }
#endif
    const auto *edge = vertex_->outputEdge(ix);
    auto rule = AllocationRule{ };
    rule.size_ = static_cast<size_t>(edge->sourceRateExpression().evaluate(handler_->getParams()));
    rule.offset_ = 0u;
    rule.fifoIx_ = 0u;
    rule.count_ = rule.size_ ? computeConsCount(edge) : 0u;
    switch (vertex_->subtype()) {
        case pisdf::VertexType::FORK:
            if (ix == 0u) {
                rule.type_ = AllocType::SAME_IN;
            } else {
                rule.offset_ = static_cast<size_t>(vertex_->outputEdge(ix - 1)->sourceRateExpression().evaluate(
                        handler_->getParams()));
                rule.fifoIx_ = static_cast<u32>(ix - 1);
                rule.type_ = AllocType::SAME_OUT;
            }
            rule.attribute_ = FifoAttribute::RW_ONLY;
            break;
        case pisdf::VertexType::DUPLICATE:
            rule.type_ = AllocType::SAME_IN;
            rule.attribute_ = FifoAttribute::RW_ONLY;
            break;
        case pisdf::VertexType::EXTERN_IN:
            rule.offset_ = vertex_->reference()->convertTo<pisdf::ExternInterface>()->bufferIndex();
            rule.type_ = AllocType::EXT;
            rule.attribute_ = FifoAttribute::RW_EXT;
            break;
        case pisdf::VertexType::REPEAT:
            if (rule.size_ ==
                static_cast<size_t>(vertex_->inputEdge(0u)->sourceRateExpression().evaluate(handler_->getParams()))) {
                auto inputFifo = fifos_->inputFifo(0u);
                rule.type_ = AllocType::SAME_IN;
                rule.attribute_ = inputFifo.attribute_;
            }
            break;
        default: {
            const auto *sink = edge->sink();
            if (sink && sink->subtype() == pisdf::VertexType::EXTERN_OUT) {
                const auto *extInterface = sink->reference()->convertTo<pisdf::ExternInterface>();
                rule.offset_ = extInterface->bufferIndex();
                rule.type_ = AllocType::EXT;
                rule.attribute_ = FifoAttribute::RW_EXT;
            }
            break;
        }
    }
    return rule;
}

spider::JobMessage spider::sched::TaskSRLess::createJobMessage() const {
    JobMessage message{ };
    /* == Set core properties == */
    message.nParamsOut_ = static_cast<u32>(vertex_->reference()->outputParamCount());
    if (vertex_->subtype() == pisdf::VertexType::INPUT) {
        message.kernelIx_ = static_cast<u32>(spider::rt::REPEAT_KERNEL_IX);
    } else {
        message.kernelIx_ = static_cast<u32>(vertex_->runtimeInformation()->kernelIx());
    }
    message.taskIx_ = static_cast<u32>(vertex_->ix()); // TODO: update this with value for CFG vertices
    message.ix_ = jobExecIx_;

    /* == Set the synchronization flags == */
    const auto lrtCount{ archi::platform()->LRTCount() };
    const auto *flags = execInfo_.notifications_.get();
    message.synchronizationFlags_ = make_unique<bool>(allocate<bool, StackID::RUNTIME>(lrtCount));
    std::copy(flags, std::next(flags, static_cast<long long>(lrtCount)), message.synchronizationFlags_.get());

    /* == Set the execution task constraints == */
    auto *execConstraints = execInfo_.constraints_.get();
    const auto numberOfConstraints{
            lrtCount - static_cast<size_t>(std::count(execConstraints, execConstraints + lrtCount, SIZE_MAX)) };
    message.execConstraints_ = array<SyncInfo>(numberOfConstraints, StackID::RUNTIME);
    auto jobIterator = std::begin(message.execConstraints_);
    for (size_t i = 0; i < lrtCount; ++i) {
        const auto value = execConstraints[i];
        if (value != SIZE_MAX) {
            jobIterator->lrtToWait_ = i;
            jobIterator->jobToWait_ = static_cast<size_t>(value);
            jobIterator++;
        }
    }

    /* == Set the input parameters (if any) == */
    message.inputParams_ = pisdf::buildVertexRuntimeInputParameters(vertex_, handler_->getParams());

    /* == Set Fifos == */
    message.fifos_ = fifos_;
    return message;
}

u32 spider::sched::TaskSRLess::color() const {
    const u32 red = static_cast<u8>((reinterpret_cast<uintptr_t>(vertex_) >> 3u) * 50 + 100);
    const u32 green = static_cast<u8>((reinterpret_cast<uintptr_t>(vertex_) >> 2u) * 50 + 100);
    const u32 blue = static_cast<u8>((reinterpret_cast<uintptr_t>(vertex_) >> 4u) * 50 + 100);
    return 0u | (red << 16u) | (green << 8u) | (blue);
}

std::string spider::sched::TaskSRLess::name() const {
    return vertex_->name() + ":" + std::to_string(firing_);
}

void spider::sched::TaskSRLess::setIx(u32 ix) noexcept {
    Task::setIx(ix);
    handler_->registerTaskIx(vertex_, firing_, ix);
}

spider::array_handle<spider::sched::Task *> spider::sched::TaskSRLess::getDependencies() const {
    return { execInfo_.dependencies_.get(), dependenciesCount_ };
}

std::pair<ufast64, ufast64> spider::sched::TaskSRLess::computeCommunicationCost(const spider::PE */*mappedPE*/) const {
    return { };
}

bool spider::sched::TaskSRLess::isMappableOnPE(const spider::PE *pe) const {
    return vertex_->runtimeInformation()->isPEMappable(pe);
}

u64 spider::sched::TaskSRLess::timingOnPE(const spider::PE *pe) const {
    return static_cast<u64>(vertex_->runtimeInformation()->timingOnPE(pe, handler_->getParams()));
}

spider::sched::DependencyInfo spider::sched::TaskSRLess::getDependencyInfo(size_t /*size*/) const {
    return { };
}

/* === Private method(s) implementation === */

size_t spider::sched::TaskSRLess::updateTaskExecutionDependency(const Schedule *schedule,
                                                                const srless::ExecDependency &dep,
                                                                size_t index) {
    if (dep.first_.vertex_ && dep.first_.vertex_->executable()) {
        index = updateTaskExecutionDependency(schedule, dep.first_, index);
    }
    if (dep.second_.vertex_ && dep.second_.vertex_->executable()) {
        index = updateTaskExecutionDependency(schedule, dep.second_, index);
    }
    return index;
}

size_t spider::sched::TaskSRLess::updateTaskExecutionDependency(const Schedule *schedule,
                                                                const srless::ExecDependencyInfo &dependencyInfo,
                                                                size_t index) {
    for (u32 k = dependencyInfo.firingStart_; k <= dependencyInfo.firingEnd_; ++k) {
        const auto taskIx = dependencyInfo.handler_->getTaskIx(dependencyInfo.vertex_, k);
        const auto &sourceTask = schedule->tasks()[taskIx];
        execInfo_.dependencies_.get()[index + k - dependencyInfo.firingStart_] = sourceTask.get();
    }
    return index + (dependencyInfo.firingEnd_ - dependencyInfo.firingStart_) + 1u;
}

template<typename T>
spider::sched::AllocationRule spider::sched::TaskSRLess::allocateInputFifo(const T &dependencies,
                                                                           const pisdf::Edge *edge) const {
    const auto count = countDependencies(dependencies);
    if (count > 1u) {
        auto rule = AllocationRule{ };
        rule.others_ = spider::allocate<AllocationRule, StackID::SCHEDULE>(count);
        rule.size_ = static_cast<size_t>(edge->sinkRateExpression().evaluate(handler_->getParams()));
        rule.offset_ = count;
        rule.fifoIx_ = UINT32_MAX;
        rule.count_ = 0u;
        rule.type_ = AllocType::MERGE;
        rule.attribute_ = FifoAttribute::R_MERGE;
        setInputFifoExtraRules(dependencies, rule.others_);
        return rule;
    }
    return allocateDefaultInputFifo(dependencies);
}

spider::sched::AllocationRule
spider::sched::TaskSRLess::allocateDefaultInputFifo(const spider::vector<srless::ExecDependency> &dependencies) const {
    return allocateDefaultInputFifo(dependencies[0u]);
}

spider::sched::AllocationRule
spider::sched::TaskSRLess::allocateDefaultInputFifo(const srless::ExecDependency &dep) const {
    auto rule = AllocationRule{ };
    rule.size_ = dep.first_.memoryEnd_ - dep.first_.memoryStart_;
    rule.offset_ = dep.first_.memoryStart_;
    rule.fifoIx_ = dep.first_.edgeIx_;
    rule.count_ = 0u;
    rule.type_ = AllocType::SAME_IN;
    rule.attribute_ = FifoAttribute::RW_OWN;
    return rule;
}

void spider::sched::TaskSRLess::setInputFifoExtraRules(const spider::vector<srless::ExecDependency> &dependencies,
                                                       AllocationRule *rules) const {
    size_t offset = 0u;
    for (auto &dep : dependencies) {
        offset = setInputFifoExtraRules(dep, rules, offset);
    }
}

size_t spider::sched::TaskSRLess::setInputFifoExtraRules(const srless::ExecDependency &dependencies,
                                                         AllocationRule *rules,
                                                         size_t offset) const {
    std::array<const srless::ExecDependencyInfo *, 2> depInfo = { &dependencies.first_, &dependencies.second_ };
    for (const auto &dep : depInfo) {
        if (dep->vertex_) {
            /* == first dependency == */
            rules[offset].others_ = nullptr;
            rules[offset].size_ = dep->memoryEnd_ - dep->memoryStart_;
            rules[offset].offset_ = dep->memoryStart_;
            rules[offset].fifoIx_ = dep->edgeIx_;
            rules[offset].count_ = 0u;
            rules[offset].type_ = spider::sched::AllocType::SAME_IN;
            rules[offset].attribute_ = spider::FifoAttribute::RW_OWN;
            const auto lowerBound = dep->firingStart_ + 1;
            const auto upperBound = dep->firingEnd_ > 0u ? dep->firingEnd_ - 1u : 0u;
            /* == middle dependencies if > 2 == */
            for (auto k = lowerBound; k <= upperBound; ++k) {
                const auto ix = k + offset - dep->firingStart_;
                rules[ix].others_ = nullptr;
                rules[ix].size_ = dep->rate_;
                rules[ix].offset_ = 0u;
                rules[ix].fifoIx_ = dep->edgeIx_;
                rules[ix].count_ = 0u;
                rules[ix].type_ = spider::sched::AllocType::SAME_IN;
                rules[ix].attribute_ = spider::FifoAttribute::RW_OWN;
            }
            /* == last dependency == */
            const auto ix = dep->firingEnd_ - dep->firingStart_ + offset;
            if (ix > offset) {
                rules[ix].others_ = nullptr;
                rules[ix].size_ = dep->memoryEnd_;
                rules[ix].offset_ = 0u;
                rules[ix].fifoIx_ = dep->edgeIx_;
                rules[ix].count_ = 0u;
                rules[ix].type_ = spider::sched::AllocType::SAME_IN;
                rules[ix].attribute_ = spider::FifoAttribute::RW_OWN;
            }
            offset += dep->firingEnd_ - dep->firingStart_ + 1u;
        }
    }
    return offset;
}

u32 spider::sched::TaskSRLess::computeConsCount(const spider::pisdf::Edge *edge) const {
    if (vertex_->subtype() == pisdf::VertexType::INPUT) {
        return recursiveConsCount(edge, handler_, 0u, handler_->getRV(edge->sink()) - 1);
    } else if (edge->sink()->hierarchical()) {
        const auto dep = handler_->computeConsDependenciesByEdge(vertex_, firing_,
                                                                 static_cast<u32>(edge->sinkPortIx()));
        return recursiveConsCount(edge, handler_, dep.first_.firingStart_, dep.first_.firingEnd_);
    } else if (edge->sink()->subtype() == pisdf::VertexType::OUTPUT) {
        return 1;
    } else {
        const auto dep = handler_->computeConsDependenciesByEdge(vertex_, firing_,
                                                                 static_cast<u32>(edge->sinkPortIx()));
        auto count = dep.first_.firingEnd_ - dep.first_.firingStart_ + 1u;
        if (dep.second_.vertex_) {
            count += (dep.second_.firingEnd_ - dep.second_.firingStart_) + 1u;
        }
        return count;
    }
}

u32 spider::sched::TaskSRLess::recursiveConsCount(const pisdf::Edge *edge,
                                                  const srless::FiringHandler *handler,
                                                  u32 firstFiring,
                                                  u32 lastFiring) const {
    if (edge->sink()->hierarchical()) {
        u32 count = 0u;
        const auto *subgraph = edge->sink()->convertTo<pisdf::Graph>();
        const auto *interface = subgraph->inputInterface(edge->sinkPortIx());
        for (auto k = firstFiring; k <= lastFiring; ++k) {
            const auto *snkHandler = handler->getChildFiring(subgraph, k);
            if (!snkHandler->isInputInterfaceTransparent(edge->sinkPortIx())) {
                count += 1;
            } else {
                count += recursiveConsCount(interface->edge(), snkHandler, 0u, handler->getRV(edge->source()) - 1);
            }
        }
        return count;
    }
    return handler->getRV(edge->source());
}
