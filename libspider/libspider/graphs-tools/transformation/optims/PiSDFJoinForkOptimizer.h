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
#ifndef SPIDER2_PISDFJOINFORKOPTIMIZER_H
#define SPIDER2_PISDFJOINFORKOPTIMIZER_H

/* === Includes === */

#include <graphs-tools/transformation/optims/PiSDFOptimizer.h>
#include <common/Math.h>
#include <spider-api/pisdf.h>

/* === Class definition === */

/**
 * @brief Optimize Join -> Fork patterns in a PiSDFGraph.
 * @see: https://tel.archives-ouvertes.fr/tel-01301642
 */
class PiSDFJoinForkOptimizer : public PiSDFOptimizer {
public:
    inline bool operator()(PiSDFGraph *graph) const override;

private:
    struct EdgeLinker {
        PiSDFVertex *vertex = nullptr;
        std::uint64_t rate = 0;
        std::uint32_t portIx = 0;

        EdgeLinker(PiSDFVertex *vertex, std::uint64_t rate, std::uint32_t portIx) : vertex{vertex},
                                                                                    rate{rate},
                                                                                    portIx{portIx} { };
    };

    inline std::uint32_t computeNJoinEdge(std::uint64_t sinkRate,
                                          Spider::Array<EdgeLinker> &sourceArray,
                                          std::uint32_t sourceIx) const;

    inline std::uint32_t computeNForkEdge(std::uint64_t sourceRate,
                                          Spider::Array<EdgeLinker> &sinkArray,
                                          std::uint32_t sinkIx) const;
};

bool PiSDFJoinForkOptimizer::operator()(PiSDFGraph *graph) const {
    Spider::vector<std::pair<PiSDFVertex *, PiSDFVertex *>> verticesToOptimize;

    /* == Search for the pair of join / fork to optimize == */
    for (auto &vertex : graph->vertices()) {
        if (vertex->type() == PiSDFVertexType::JOIN) {
            auto *sink = vertex->outputEdge(0)->sink();
            if (sink->type() == PiSDFVertexType::FORK) {
                verticesToOptimize.push_back(std::make_pair(vertex, sink));
            }
        }
    }

    /* == Go through the different pair to optimize == */
    for (auto &pair : verticesToOptimize) {
        auto *join = pair.first;
        auto *fork = pair.second;
        Spider::Array<EdgeLinker> sourceArray{join->nEdgesIN(), StackID::TRANSFO};
        Spider::Array<EdgeLinker> sinkArray{fork->nEdgesOUT(), StackID::TRANSFO};

        for (auto *edge : join->inputEdges()) {
            sourceArray[edge->sinkPortIx()] = EdgeLinker{edge->source(), edge->sourceRate(), edge->sourcePortIx()};
            graph->removeEdge(edge);
        }
        graph->removeEdge(join->outputEdge(0));
        for (auto *edge : fork->outputEdges()) {
            sinkArray[edge->sourcePortIx()] = EdgeLinker{edge->sink(), edge->sinkRate(), edge->sinkPortIx()};
            graph->removeEdge(edge);
        }

        /* == Remove fork / join == */
        graph->removeVertex(join);
        graph->removeVertex(fork);

        /* == Re-do the linking == */
        std::uint32_t sourceIx = 0;
        std::uint32_t forkEdgeIx = 0;
        for (std::uint64_t sinkIx = 0; sinkIx < sinkArray.size(); ++sinkIx) {
            auto &sink = sinkArray[sinkIx];
            auto &source = sourceArray[sourceIx];
            if (sink.rate == source.rate) {
                auto sourcePortIx = source.portIx == UINT32_MAX ? forkEdgeIx : source.portIx;
                Spider::API::createEdge(graph, source.vertex, sourcePortIx, source.rate,
                                        sink.vertex, sink.portIx, sink.rate,
                                        StackID::TRANSFO);
                sourceIx += 1;
            } else if (source.rate > sink.rate) {
                if (source.portIx == UINT32_MAX) {
                    /* == Case of added Fork == */
                    Spider::API::createEdge(graph, source.vertex, forkEdgeIx, sink.rate,
                                            sink.vertex, sink.portIx, sink.rate,
                                            StackID::TRANSFO);
                    source.rate -= sink.rate;
                    forkEdgeIx += 1;
                } else {
                    /* == Add a Fork == */
                    auto nForkEdge = computeNForkEdge(source.rate, sinkArray, sinkIx);
                    auto *addedFork = Spider::API::createFork(graph,
                                                              "fork-" + source.vertex->name() + "-out" +
                                                              std::to_string(source.portIx),
                                                              nForkEdge,
                                                              0, StackID::TRANSFO);
                    Spider::API::createEdge(graph, source.vertex, source.portIx, source.rate,
                                            addedFork, 0, source.rate,
                                            StackID::TRANSFO);
                    Spider::API::createEdge(graph, addedFork, 0, sink.rate,
                                            sink.vertex, sink.portIx, sink.rate,
                                            StackID::TRANSFO);
                    source.vertex = addedFork;
                    source.portIx = UINT32_MAX;
                    source.rate -= sink.rate;
                    forkEdgeIx = 1;
                }
            } else {
                /* == Need for a Join == */
                auto nJoinEdge = computeNJoinEdge(sink.rate, sourceArray, sourceIx);
                auto *addedJoin = Spider::API::createJoin(graph,
                                                          "join-" + sink.vertex->name() + "-in" +
                                                          std::to_string(sink.portIx),
                                                          nJoinEdge,
                                                          0, StackID::TRANSFO);
                Spider::API::createEdge(graph, addedJoin, 0, sink.rate,
                                        sink.vertex, sink.portIx, sink.rate,
                                        StackID::TRANSFO);
                for (std::uint64_t joinPortIx = 0; joinPortIx < addedJoin->nEdgesIN(); ++joinPortIx) {
                    source = sourceArray[sourceIx];
                    if (source.rate <= sink.rate) {
                        auto sourcePortIx = source.portIx == UINT32_MAX ? forkEdgeIx : source.portIx;
                        Spider::API::createEdge(graph, source.vertex, sourcePortIx, source.rate,
                                                addedJoin, joinPortIx, source.rate,
                                                StackID::TRANSFO);
                        sink.rate -= source.rate;
                        sourceIx += 1;
                    } else {
                        sink.vertex = addedJoin;
                        sink.portIx = joinPortIx;
                        sinkIx -= 1;
                    }
                }
            }
        }
    }
    return verticesToOptimize.empty();
}

std::uint32_t PiSDFJoinForkOptimizer::computeNJoinEdge(std::uint64_t sinkRate,
                                                       Spider::Array<EdgeLinker> &sourceArray,
                                                       std::uint32_t sourceIx) const {
    std::uint32_t nJoinEdge = 0;
    std::uint64_t totalRate = 0;
    while (sinkRate > totalRate) {
        totalRate += sourceArray[sourceIx].rate;
        nJoinEdge += 1;
        sourceIx += 1;
    }
    return nJoinEdge;
}

std::uint32_t PiSDFJoinForkOptimizer::computeNForkEdge(std::uint64_t sourceRate,
                                                       Spider::Array<EdgeLinker> &sinkArray,
                                                       std::uint32_t sinkIx) const {
    std::uint32_t nForkEdge = 0;
    std::uint64_t totalRate = 0;
    while (sourceRate > totalRate) {
        totalRate += sinkArray[sinkIx].rate;
        nForkEdge += 1;
        sinkIx += 1;
    }
    return nForkEdge;
}

#endif //SPIDER2_PISDFJOINFORKOPTIMIZER_H
