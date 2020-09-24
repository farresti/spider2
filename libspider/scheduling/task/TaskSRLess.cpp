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

/* === Static function === */

/* === Method(s) implementation === */

spider::sched::TaskSRLess::TaskSRLess(srless::FiringHandler *handler,
                                      const pisdf::Vertex *vertex,
                                      u32 firing,
                                      const spider::vector<spider::srless::ExecDependency> &dependencies) :
        Task(),
        handler_{ handler },
        vertex_{ vertex },
        firing_{ firing },
        dependenciesCount_{ 0u } {
    size_t mergedFifoCount{ 0u };
    for (const auto &dep : dependencies) {
        const auto diff = dep.firingEnd_ - dep.firingStart_;
        dependenciesCount_ += diff + 1u;
        mergedFifoCount += (diff > 0);
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
    const auto dependencies = handler_->computeExecDependenciesByFiring(vertex_, firing_);
    for (const auto &dep : dependencies) {
        if (dep.vertex_ && dep.vertex_->executable()) {
            for (u32 k = dep.firingStart_; k <= dep.firingEnd_; ++k) {
                const auto taskIx = dep.handler_->getTaskIx(dep.vertex_, k);
                const auto &sourceTask = schedule->tasks()[taskIx];
                execInfo_.dependencies_.get()[i + k - dep.firingStart_] = sourceTask.get();
            }
        }
        i += (dep.firingEnd_ - dep.firingStart_) + 1u;
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
    const auto dependencies = handler_->computeExecDependenciesByEdge(vertex_, firing_, static_cast<u32>(ix));
    size_t count = 0u;
    for (const auto &dep : dependencies) {
        count += (dep.firingEnd_ - dep.firingStart_) + 1u;
    }
    if (count > 1u) {
        auto rule = AllocationRule{ };
        rule.others_ = spider::allocate<AllocationRule, StackID::SCHEDULE>(count);
        rule.offset_ = count;
        rule.size_ = static_cast<size_t>(vertex_->inputEdge(ix)->sinkRateExpression().evaluate(handler_->getParams()));
        rule.type_ = AllocType::MERGE;
        rule.attribute_ = FifoAttribute::RW_MERGE;
        size_t i = 0u;
        for (const auto &dep : dependencies) {
            if (dep.vertex_) {
                rule.others_[i] = { nullptr,
                                    dep.memory_.rate_ - dep.memory_.memoryStart_,
                                    dep.memory_.memoryStart_,
                                    dep.memory_.edgeIx_,
                                    spider::sched::AllocType::SAME_IN,
                                    spider::FifoAttribute::RW_OWN };
                for (auto k = dep.firingStart_ + 1; k <= dep.firingEnd_ - 1; ++k) {
                    const auto offsetIx = k - dep.firingStart_;
                    rule.others_[i + offsetIx] = { nullptr,
                                                   dep.memory_.rate_,
                                                   0u,
                                                   dep.memory_.edgeIx_,
                                                   spider::sched::AllocType::SAME_IN,
                                                   spider::FifoAttribute::RW_OWN };
                }
                rule.others_[i + (dep.firingEnd_ - dep.firingStart_)] = { nullptr,
                                                                          dep.memory_.memoryEnd_,
                                                                          0u,
                                                                          dep.memory_.edgeIx_,
                                                                          spider::sched::AllocType::SAME_IN,
                                                                          spider::FifoAttribute::RW_OWN };
                i += (dep.firingEnd_ - dep.firingStart_) + 1u;
            }
        }
        return rule;
    } else {
        const auto &dep = dependencies[0u];
        const auto rate = dep.memory_.memoryEnd_ - dep.memory_.memoryStart_;
        return { nullptr, rate, dep.memory_.memoryStart_, dep.memory_.edgeIx_, spider::sched::AllocType::SAME_IN,
                 spider::FifoAttribute::RW_OWN };
    }
}

spider::sched::AllocationRule spider::sched::TaskSRLess::allocationRuleForOutputFifo(size_t ix) const {
#ifndef NDEBUG
    if (ix >= vertex_->outputEdgeCount()) {
        throwSpiderException("index out of bound.");
    }
#endif
    const auto *edge = vertex_->outputEdge(ix);
    const auto rate = static_cast<size_t>(edge->sourceRateExpression().evaluate(handler_->getParams()));
    switch (vertex_->subtype()) {
        case pisdf::VertexType::FORK:
            if (ix == 0u) {
                return { nullptr, rate, 0u, 0u, AllocType::SAME_IN, FifoAttribute::RW_ONLY };
            } else {
                const auto offset = vertex_->outputEdge(ix - 1)->sourceRateExpression().evaluate(handler_->getParams());
                return { nullptr, rate, static_cast<size_t>(offset), static_cast<u32>(ix - 1), AllocType::SAME_OUT,
                         FifoAttribute::RW_ONLY };
            }
        case pisdf::VertexType::DUPLICATE:
            return { nullptr, rate, 0u, 0u, AllocType::SAME_IN, FifoAttribute::RW_ONLY };
        case pisdf::VertexType::EXTERN_IN: {
            const auto ref = vertex_->reference()->convertTo<pisdf::ExternInterface>();
            return { nullptr, rate, ref->bufferIndex(), 0u, AllocType::EXT, FifoAttribute::RW_EXT };
        }
        case pisdf::VertexType::REPEAT: {
            const auto outputRate = vertex_->inputEdge(0u)->sourceRateExpression().evaluate(handler_->getParams());
            if (rate == static_cast<size_t>(outputRate)) {
                auto inputFifo = fifos_->inputFifo(0u);
                return { nullptr, rate, 0u, 0u, AllocType::SAME_IN, inputFifo.attribute_ };
            }
            return { nullptr, rate, 0u, UINT32_MAX, AllocType::NEW, FifoAttribute::RW_OWN };
        }
        default: {
            const auto *sink = edge->sink();
            if (sink && sink->subtype() == pisdf::VertexType::EXTERN_OUT) {
                const auto *extInterface = sink->reference()->convertTo<pisdf::ExternInterface>();
                return { nullptr, rate, extInterface->bufferIndex(), 0u, AllocType::EXT, FifoAttribute::RW_EXT };
            }
            return { nullptr, rate, 0u, UINT32_MAX, AllocType::NEW, FifoAttribute::RW_OWN };
        }
    }
}

spider::JobMessage spider::sched::TaskSRLess::createJobMessage() const {
    JobMessage message{ };
    /* == Set core properties == */
    message.nParamsOut_ = static_cast<u32>(vertex_->reference()->outputParamCount());
    message.kernelIx_ = static_cast<u32>(vertex_->runtimeInformation()->kernelIx());
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
//    return { vertex_->inputEdge(ix)->sourcePortIx(),
//             static_cast<size_t>(vertex_->inputEdge(ix)->sourceRateValue()) };
}

/* === Private method(s) implementation === */
