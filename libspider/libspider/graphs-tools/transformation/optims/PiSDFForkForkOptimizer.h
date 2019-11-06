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
#ifndef SPIDER2_PISDFFORKFORKOPTIMIZER_H
#define SPIDER2_PISDFFORKFORKOPTIMIZER_H

/* === Includes === */

#include <graphs-tools/transformation/optims/PiSDFOptimizer.h>
#include <graphs/pisdf/specials/Specials.h>

/* === Class definition === */

/**
 * @brief Optimize Fork -> Fork patterns in a PiSDFGraph.
 * @see: https://tel.archives-ouvertes.fr/tel-01301642
 */
class PiSDFForkForkOptimizer final : public PiSDFOptimizer {
public:
    inline bool operator()(PiSDFGraph *graph) const override;
};

bool PiSDFForkForkOptimizer::operator()(PiSDFGraph *graph) const {
    Spider::vector<std::pair<Spider::PiSDF::ForkVertex *, Spider::PiSDF::ForkVertex *>> verticesToOptimize;

    /* == Search for the pair of fork to optimize == */
    for (const auto &v : graph->vertices()) {
        if (v->subtype() == Spider::PiSDF::VertexType::FORK) {
            /* == return itself, only way to suppress the warning for "probably incompatible type cast" == */
            const auto &vertex = v->inputEdge(0)->sink();
            auto *source = vertex->inputEdge(0)->source();
            if (source->subtype() == Spider::PiSDF::VertexType::FORK) {
                verticesToOptimize.push_back(std::make_pair(dynamic_cast<Spider::PiSDF::ForkVertex *>(source),
                                                            dynamic_cast<Spider::PiSDF::ForkVertex *>(vertex)));
            }
        }
    }

    /* == Do the optimization == */
    const auto &params = graph->params();
    for (auto it = verticesToOptimize.begin(); it != verticesToOptimize.end(); ++it) {
        auto &pair = (*it);
        auto *source = pair.first;
        auto *vertex = pair.second;

        /* == Create the new fork == */
        auto *fork = Spider::API::createFork(graph,
                                             "merged-" + source->name() + "-" + vertex->name(),
                                             (source->edgesOUTCount() - 1) + vertex->edgesOUTCount(),
                                             0,
                                             StackID::TRANSFO);
        auto *edge = source->inputEdge(0);
        auto rate = edge->sinkRateExpression().evaluate(params);
        edge->setSource(fork, 0, Expression(rate));

        /* == Link the edges == */
        auto insertEdgeIx = vertex->inputEdge(0)->sourcePortIx();
        std::uint32_t offset = 0;
        for (auto *sourceEdge : source->outputEdgeArray()) {
            if (sourceEdge->sourcePortIx() == insertEdgeIx) {
                graph->removeEdge(sourceEdge);
                offset += vertex->edgesOUTCount() - 1;
                for (auto *vertexEdge : vertex->outputEdgeArray()) {
                    rate = vertexEdge->sourceRateExpression().evaluate(params);
                    auto ix = vertexEdge->sourcePortIx() + insertEdgeIx;
                    vertexEdge->setSource(fork, ix, Expression(rate));
                }
            } else {
                rate = sourceEdge->sourceRateExpression().evaluate(params);
                auto ix = sourceEdge->sourcePortIx() + offset;
                sourceEdge->setSource(fork, ix, Expression(rate));
            }
        }

        /* == Search for the pair to modify (if any) == */
        for (auto it2 = std::next(it); it2 != std::end(verticesToOptimize); ++it2) {
            auto &secPair = (*it2);
            if (secPair.first == vertex || secPair.first == source) {
                secPair.first = fork;
            }
            if (secPair.second == source || secPair.second == vertex) {
                secPair.second = fork;
            }
        }

        /* == Remove the vertices == */
        Spider::Logger::printVerbose(LOG_OPTIMS, "ForkForkOptimizer: removing [%s] and [%s] fork vertices.\n",
                                     vertex->name().c_str(), source->name().c_str());
        graph->removeVertex(vertex);
        graph->removeVertex(source);
    }

    return verticesToOptimize.empty();
}

#endif //SPIDER2_PISDFFORKFORKOPTIMIZER_H
