/*
 * Copyright or © or Copr. IETR/INSA - Rennes (2013 - 2019) :
 *
 * Antoine Morvan <antoine.morvan@insa-rennes.fr> (2018)
 * Clément Guy <clement.guy@insa-rennes.fr> (2014)
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2017-2019)
 * Hugo Miomandre <hugo.miomandre@insa-rennes.fr> (2017)
 * Julien Heulot <julien.heulot@insa-rennes.fr> (2013 - 2015)
 * Yaset Oliva <yaset.oliva@insa-rennes.fr> (2013 - 2014)
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

/* === Includes === */

#include <cinttypes>
#include <graphs-tools/transformation/SRDAGTransformation.h>
#include <graphs/pisdf/params/DynamicParam.h>
#include <graphs/pisdf/Graph.h>
#include <graphs/pisdf/Edge.h>
#include <graphs/pisdf/Delay.h>
#include <graphs/pisdf/ExecVertex.h>
#include <graphs/pisdf/specials/Specials.h>
#include <graphs/pisdf/interfaces/InputInterface.h>
#include <graphs/pisdf/interfaces/OutputInterface.h>
#include <graphs-tools/brv/LCMBRVCompute.h>
#include <graphs-tools/numerical/PiSDFAnalysis.h>

/* === Static function(s) === */

static PiSDFAbstractVertex *fetchOrClone(const PiSDFAbstractVertex *reference, Spider::SRDAG::EdgeLinker &linker) {
    if (!reference) {
        throwSpiderException("Trying to clone nullptr vertex.");
    }
    auto referenceIx = reference->ix();
    if (reference->subtype() == PiSDFVertexType::INPUT) {
        referenceIx += linker.job_.reference_->vertexCount();
    } else if (reference->subtype() == PiSDFVertexType::OUTPUT) {
        referenceIx += linker.job_.reference_->vertexCount();
        referenceIx += linker.job_.reference_->edgesINCount();
    }
    auto &index = linker.tracker_[referenceIx];

    // TODO: handle the split of dynamic graphs

    /* == If vertex has already been cloned return the first one == */
    if (index != UINT32_MAX) {
        return linker.srdag_->vertex(index);
    }

    if (reference->subtype() == PiSDFVertexType::GRAPH) {
        // TODO split dynamic graphs
        const auto *graph = dynamic_cast<const PiSDFGraph *>(reference);
        if (!graph->dynamic()) {
            for (std::uint32_t it = 0; it < reference->repetitionValue(); ++it) {
                auto *vertex = Spider::API::createVertex(linker.srdag_,
                                                         reference->name() + "_" + std::to_string(it),
                                                         reference->edgesINCount(),
                                                         reference->edgesOUTCount(),
                                                         StackID::TRANSFO);
                linker.nextJobs_.emplace_back(graph, vertex->ix(), it);
                index = vertex->ix();
            }
        }
    } else {
        /* == Clone the vertex N times and return the first one == */
        for (std::uint32_t it = 0; it < reference->repetitionValue(); ++it) {
            auto *vertex = reference->clone(StackID::TRANSFO, linker.srdag_);
            vertex->setName(vertex->name() + "_" + std::to_string(it));
            index = vertex->ix();
        }
    }
    index = index - (reference->repetitionValue() - 1);
    return linker.srdag_->vertex(index);
}

static void pushReverseVertexLinkerVector(Spider::SRDAG::LinkerVector &vector,
                                          const PiSDFAbstractVertex *reference,
                                          const std::int64_t &rate,
                                          const std::uint32_t portIx,
                                          Spider::SRDAG::EdgeLinker &linker) {
    using Spider::SRDAG::VertexLinker;
    const auto &cloneIx = fetchOrClone(reference, linker)->ix();
    for (auto i = (cloneIx + reference->repetitionValue()); i != cloneIx; --i) {
        vector.emplace_back(rate, portIx, linker.srdag_->vertex(i - 1));
    }
}

static Spider::SRDAG::LinkerVector buildSourceLinkerVector(Spider::SRDAG::EdgeLinker &linker) {
    const auto &edge = linker.edge_;
    const auto &source = edge->source();
    const auto &delay = edge->delay();
    Spider::SRDAG::LinkerVector sourceVector;
    sourceVector.reserve(source->repetitionValue() + (delay ? delay->setter()->repetitionValue() : 0));

    /* == Populate first the source clones in reverse order == */
    const auto &rate = source->type() == PiSDFVertexType::INTERFACE ? edge->sinkRateExpression().evaluate() *
                                                                      edge->sink()->repetitionValue()
                                                                    : edge->sourceRateExpression().evaluate();
    pushReverseVertexLinkerVector(sourceVector, source, rate, edge->sourcePortIx(), linker);

    /* == If delay, populate the setter clones in reverse order == */
    if (delay) {
        const auto &setterEdge = delay->vertex()->inputEdge(0);
        const auto &setter = delay->setter();
        const auto &setterRate = setterEdge->sourceRateExpression().evaluate();
        pushReverseVertexLinkerVector(sourceVector, setter, setterRate, setterEdge->sourcePortIx(), linker);
    }
    return sourceVector;
}

static Spider::SRDAG::LinkerVector buildSinkLinkerVector(Spider::SRDAG::EdgeLinker &linker) {
    const auto &edge = linker.edge_;
    const auto &sink = edge->sink();
    const auto &delay = edge->delay();

    Spider::SRDAG::LinkerVector sinkVector;
    sinkVector.reserve(sink->repetitionValue() + (delay ? delay->getter()->repetitionValue() : 0));

    /* == First, if delay, populate the getter clones in reverse order == */
    if (delay) {
        const auto &getterEdge = delay->vertex()->outputEdge(0);
        const auto &getter = delay->getter();
        const auto &getterRate = getterEdge->sinkRateExpression().evaluate();
        pushReverseVertexLinkerVector(sinkVector, getter, getterRate, getterEdge->sinkPortIx(), linker);
    }

    /* == Populate the sink clones in reverse order == */
    const auto &rate = sink->type() == PiSDFVertexType::INTERFACE ? edge->sourceRateExpression().evaluate() *
                                                                    edge->source()->repetitionValue()
                                                                  : edge->sinkRateExpression().evaluate();
    pushReverseVertexLinkerVector(sinkVector, sink, rate, edge->sinkPortIx(), linker);
    return sinkVector;
}

static void computeDependencies(Spider::SRDAG::LinkerVector &srcVector,
                                Spider::SRDAG::LinkerVector &snkVector,
                                const PiSDFEdge *edge) {
    auto &&delay = edge->delay() ? edge->delay()->value() : 0;
    const auto &srcRate = srcVector[0].rate_;     /* = This should be the proper source rate of the edge = */
    const auto &snkRate = snkVector.back().rate_; /* = This should be the proper sink rate of the edge = */
    const auto &setterRate = edge->delay() ? srcVector.back().rate_ : 0;
    const auto &getterRate = edge->delay() ? snkVector[0].rate_ : 0;
    const auto &sinkRepetitionValue = edge->sink()->repetitionValue();
    const auto &setterOffset = edge->delay() ? edge->delay()->setter()->repetitionValue() : 0;

    /* == Compute dependencies for sinks == */
    std::uint32_t firing = 0;
    auto currentSinkRate = snkRate;
    for (auto it = snkVector.rbegin(); it < snkVector.rend(); ++it) {
        if (it == snkVector.rbegin() + sinkRepetitionValue) {
            /* == We've reached the end / getter vertices == */
            delay = delay - snkRate * sinkRepetitionValue;
            currentSinkRate = getterRate;
            firing = 0;
        }
        auto snkLowerDep = Spider::PiSDF::computeConsLowerDep(currentSinkRate, srcRate, firing, delay);
        auto snkUpperDep = Spider::PiSDF::computeConsUpperDep(currentSinkRate, srcRate, firing, delay);
        if (snkLowerDep < 0) {
            /* == Update dependencies for init / setter == */
            snkLowerDep -= Spider::PiSDF::computeConsLowerDep(snkRate, setterRate, firing, 0);
            if (snkUpperDep < 0) {
                snkUpperDep -= Spider::PiSDF::computeConsUpperDep(snkRate, setterRate, firing, 0);
            }
        }

        /* == Adjust the values to match the actual position in the source vector == */
        snkLowerDep += setterOffset;
        snkUpperDep += setterOffset;
        (*it).lowerDep_ = snkLowerDep;
        (*it).upperDep_ = snkUpperDep;
        firing += 1;
    }

    /* == Update source vector with proper dependencies == */
    firing = 0;
    for (auto it = snkVector.rbegin(); it < snkVector.rend(); ++it) {
        const auto &lowerIndex = srcVector.size() - 1 - (*it).lowerDep_;
        const auto &upperIndex = srcVector.size() - 1 - (*it).upperDep_;
        srcVector[lowerIndex].lowerDep_ = std::min(srcVector[lowerIndex].lowerDep_, firing);
        srcVector[lowerIndex].upperDep_ = std::max(srcVector[lowerIndex].upperDep_, firing);
        srcVector[upperIndex].lowerDep_ = std::min(srcVector[upperIndex].lowerDep_, firing);
        srcVector[upperIndex].upperDep_ = std::max(srcVector[upperIndex].upperDep_, firing);
        firing += 1;
    }
}

static void addForkVertex(Spider::SRDAG::LinkerVector &srcVector,
                          Spider::SRDAG::LinkerVector &snkVector,
                          PiSDFGraph *srdag) {
    const auto &sourceLinker = srcVector.back();
    auto *fork = Spider::API::createFork(srdag,
                                         "fork-" + sourceLinker.vertex_->name() + "_out-" +
                                         std::to_string(sourceLinker.portIx_),
                                         (sourceLinker.upperDep_ - sourceLinker.lowerDep_) + 1,
                                         0,
                                         StackID::TRANSFO);

    /* == Create an edge between source and fork == */
    Spider::API::createEdge(sourceLinker.vertex_, sourceLinker.portIx_, sourceLinker.rate_,
                            fork, 0, sourceLinker.rate_, StackID::TRANSFO);
    srcVector.pop_back();

    /* == Connect out of fork == */
    auto remaining = sourceLinker.rate_;
    for (std::uint32_t i = 0; i < fork->edgesOUTCount() - 1; ++i) {
        const auto &sinkLinker = snkVector.back();
        remaining -= sinkLinker.rate_;
        Spider::API::createEdge(fork, i, sinkLinker.rate_,
                                sinkLinker.vertex_, sinkLinker.portIx_, sinkLinker.rate_, StackID::TRANSFO);
        snkVector.pop_back();
    }
    srcVector.emplace_back(remaining, fork->edgesOUTCount() - 1, fork);
    srcVector.back().lowerDep_ = sourceLinker.upperDep_;
    srcVector.back().upperDep_ = sourceLinker.upperDep_;
}

static void addJoinVertex(Spider::SRDAG::LinkerVector &srcVector,
                          Spider::SRDAG::LinkerVector &snkVector,
                          PiSDFGraph *srdag) {
    const auto &sinkLinker = snkVector.back();
    auto *join = Spider::API::createJoin(srdag,
                                         "join-" + sinkLinker.vertex_->name() + "_in-" +
                                         std::to_string(sinkLinker.portIx_),
                                         (sinkLinker.upperDep_ - sinkLinker.lowerDep_) + 1,
                                         StackID::TRANSFO);

    /* == Create an edge between source and fork == */
    Spider::API::createEdge(join, 0, sinkLinker.rate_,
                            sinkLinker.vertex_, sinkLinker.portIx_, sinkLinker.rate_, StackID::TRANSFO);
    snkVector.pop_back();

    /* == Connect in of join == */
    auto remaining = sinkLinker.rate_;
    for (std::uint32_t i = 0; i < join->edgesINCount() - 1; ++i) {
        const auto &sourceLinker = srcVector.back();
        remaining -= sourceLinker.rate_;
        Spider::API::createEdge(sourceLinker.vertex_, sourceLinker.portIx_, sourceLinker.rate_,
                                join, i, sourceLinker.rate_, StackID::TRANSFO);
        srcVector.pop_back();
    }
    snkVector.emplace_back(remaining, join->edgesINCount() - 1, join);
    snkVector.back().lowerDep_ = sinkLinker.upperDep_;
    snkVector.back().upperDep_ = sinkLinker.upperDep_;
}

static void replaceJobInterfaces(Spider::SRDAG::EdgeLinker &linker) {
    if (linker.job_.instanceValue_ == UINT32_MAX) {
        return;
    }
    auto *srdagInstance = linker.srdag_->vertex(linker.job_.srdagIx_);
    if (!srdagInstance) {
        throwSpiderException("could not find matching single rate instance [%"
                                     PRIu32
                                     "] of graph [%s]", linker.job_.instanceValue_,
                             linker.job_.reference_->name().c_str());
    }

    /* == Replace the input interfaces == */
    for (const auto &interface : linker.job_.reference_->inputInterfaces()) {
        auto *edge = srdagInstance->inputEdge(interface->ix());
        auto *vertex = Spider::API::createUpsample(linker.srdag_,
                                                   srdagInstance->name() + "_" + interface->name(),
                                                   0,
                                                   StackID::TRANSFO);
        edge->setSink(vertex, 0, Expression(edge->sinkRateExpression()));
        linker.tracker_[linker.job_.reference_->vertexCount() + interface->ix()] = vertex->ix();
    }

    /* == Replace the output interfaces == */
    for (const auto &interface : linker.job_.reference_->outputInterfaces()) {
        auto *edge = srdagInstance->outputEdge(interface->ix());
        auto *vertex = Spider::API::createDownsample(linker.srdag_,
                                                     srdagInstance->name() + "_" + interface->name(),
                                                     0,
                                                     StackID::TRANSFO);
        edge->setSource(vertex, 0, Expression(edge->sourceRateExpression()));
        linker.tracker_[linker.job_.reference_->vertexCount() +
                        linker.job_.reference_->edgesINCount() +
                        interface->ix()] = vertex->ix();
    }
}

/* === Methods implementation === */

std::pair<Spider::SRDAG::JobStack, Spider::SRDAG::JobStack>
Spider::SRDAG::staticSingleRateTransformation(const Spider::SRDAG::Job &job, PiSDFGraph *srdag) {
    if (!srdag) {
        throwSpiderException("nullptr for single rate graph.");
    }
    if (!job.reference_) {
        throwSpiderException("nullptr for job.reference graph.");
    }

    /* == Compute the repetition values of the graph (if dynamic and/or first instance) == */
    // TODO: update parameters values of job.reference_ using the one of current instance
    if (job.reference_->dynamic() || job.instanceValue_ == 0 || job.instanceValue_ == UINT32_MAX) {
        LCMBRVCompute brvTask{job.reference_};
        brvTask.execute();
    }

    TransfoTracker vertexTransfoTracker;
    vertexTransfoTracker.reserve(job.reference_->vertexCount() +
                                 job.reference_->edgesINCount() +
                                 job.reference_->edgesOUTCount());
    for (size_t i = 0; i < vertexTransfoTracker.capacity(); ++i) {
        vertexTransfoTracker.push_back(UINT32_MAX);
    }
    JobStack nextJobs;
    JobStack dynaJobs;
    auto linker = EdgeLinker{nullptr, srdag, job, nextJobs, dynaJobs, vertexTransfoTracker};

    /* == Replace the interfaces of the graph and remove the vertex == */
    replaceJobInterfaces(linker);

    /* == Do the linkage for every edges of the graph == */
    for (const auto &edge : job.reference_->edges()) {
        linker.edge_ = edge;
        if (edge->source()->type() != PiSDFVertexType::DELAY &&
            edge->sink()->type() != PiSDFVertexType::DELAY) {
            staticEdgeSingleRateLinkage(linker);
        }
    }

    /* == Check for non-connected vertices == */
    linker.edge_ = nullptr;
    for (const auto &vertex : job.reference_->vertices()) {
        if (vertex->type() != PiSDFVertexType::DELAY) {
            fetchOrClone(vertex, linker);
        }
    }

    /* == Remove the vertex from the srdag == */
    if (job.instanceValue_ != UINT32_MAX) {
        auto *srdagInstance = linker.srdag_->vertex(linker.job_.srdagIx_);
        linker.srdag_->removeVertex(srdagInstance);
    }

    return std::make_pair(std::move(nextJobs), std::move(dynaJobs));
}

void Spider::SRDAG::staticEdgeSingleRateLinkage(EdgeLinker &linker) {
    if ((linker.edge_->source() == linker.edge_->sink())) {
        if (!linker.edge_->delay()) {
            throwSpiderException("No delay on edge with self loop.");
        } else if (linker.edge_->delay()->value() < linker.edge_->sinkRateExpression().evaluate()) {
            throwSpiderException("Insufficient delay [%"
                                         PRIu32
                                         "] on edge [%s].", linker.edge_->delay()->value(),
                                 linker.edge_->name().c_str());
        }
    }

    auto sourceVector = buildSourceLinkerVector(linker);
    auto sinkVector = buildSinkLinkerVector(linker);

    /* == Compute the different dependencies of sinks over sources == */
    computeDependencies(sourceVector, sinkVector, linker.edge_);

    /* == Iterate over sinks == */
    while (!sinkVector.empty()) {
        const auto &snkLnk = sinkVector.back();
        const auto &srcLnk = sourceVector.back();
        if (snkLnk.lowerDep_ == snkLnk.upperDep_) {
            if (srcLnk.lowerDep_ == srcLnk.upperDep_) {
                /* == Forward link between source and sink == */
                Spider::API::createEdge(srcLnk.vertex_, srcLnk.portIx_, srcLnk.rate_,
                                        snkLnk.vertex_, snkLnk.portIx_, snkLnk.rate_, StackID::TRANSFO);
                sourceVector.pop_back();
                sinkVector.pop_back();
            } else {
                /* == Source need a fork == */
                addForkVertex(sourceVector, sinkVector, linker.srdag_);
            }
        } else {
            /* == Sink need a join == */
            addJoinVertex(sourceVector, sinkVector, linker.srdag_);
        }
    }

    /* == Sanity check == */
    if (!sourceVector.empty()) {
        throwSpiderException("remaining sources to link after single rate transformation on edge: [%s].",
                             linker.edge_->name().c_str());
    }
}
