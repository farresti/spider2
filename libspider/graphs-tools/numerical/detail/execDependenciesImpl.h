/*
 * Copyright or © or Copr. IETR/INSA - Rennes (2020) :
 *
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2020)
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
#ifndef SPIDER2_EXECDEPENDENCIESIMPL_H
#define SPIDER2_EXECDEPENDENCIESIMPL_H

/* === Include(s) === */

#include <graphs-tools/numerical/detail/DependencyIterator.h>
#include <graphs-tools/transformation/pisdf/GraphHandler.h>
#include <graphs-tools/transformation/pisdf/GraphFiring.h>
#include <graphs/pisdf/Graph.h>
#include <graphs/pisdf/DelayVertex.h>

namespace spider {
    namespace pisdf {

        class Vertex;

        class Edge;

        namespace detail {

            /* === Function(s) prototype === */

            template<class ...Args>
            i32 computeExecDependency(const Edge *edge,
                                      int64_t lowerCons,
                                      int64_t upperCons,
                                      const GraphFiring *handler,
                                      Args &&...args);

            namespace impl {

                inline DependencyInfo createExecDependency(const Edge *edge,
                                                           int64_t lowerCons,
                                                           int64_t upperCons,
                                                           int64_t srcRate,
                                                           int64_t delayValue,
                                                           const pisdf::GraphFiring *handler) {
                    if (!srcRate) {
                        return { nullptr, nullptr, 0, 0, 0, 0, 0, 0 };
                    }
                    DependencyInfo dep{ };
                    dep.vertex_ = edge->source();
                    dep.handler_ = handler;
                    dep.rate_ = srcRate;
                    dep.edgeIx_ = static_cast<u32>(edge->sourcePortIx());
                    const auto delayedLowerCons = lowerCons - delayValue;
                    const auto startDiv = delayedLowerCons / srcRate;
                    const auto memStart = delayedLowerCons % srcRate;
                    const auto delayedUpperCons = upperCons - delayValue;
                    const auto endDiv = delayedUpperCons / srcRate;
                    const auto memEnd = delayedUpperCons % srcRate;
                    /* == floor div == */
                    const auto start = memStart ? startDiv - (delayedLowerCons < 0) : startDiv;
                    const auto end = memEnd ? endDiv - (delayedUpperCons < 0) : endDiv;
                    dep.memoryStart_ = static_cast<u32>(memStart);
                    dep.memoryEnd_ = static_cast<u32>(memEnd);
                    dep.firingStart_ = static_cast<u32>(start);
                    dep.firingEnd_ = static_cast<u32>(end);
                    return dep;
                }

                inline void apply(const DependencyInfo &) { }

                inline void apply(const DependencyInfo &dep, spider::vector<DependencyInfo> &result) {
                    spider::reserve(result, 1u);
                    result.emplace_back(dep);
                }

                template<class Function, class ...Args>
                inline void apply(const DependencyInfo &dep, const Function &func, Args &&...args) {
                    func(dep, std::forward<Args>(args)...);
                }

                template<class ...Args>
                i32 computeExecDependencyGetter(const Edge *edge,
                                                int64_t lowerCons,
                                                int64_t upperCons,
                                                const pisdf::GraphFiring *handler,
                                                Args &&...args) {
                    /* == Case of getter vertex == */
                    const auto *source = edge->source();
                    const auto srcRate = handler->getSrcRate(edge);
                    const auto *delayFromVertex = source->convertTo<DelayVertex>()->delay();
                    const auto *delayEdge = delayFromVertex->edge();
                    const auto *sink = delayEdge->sink();
                    const auto snkRate = handler->getSnkRate(delayEdge);
                    const auto snkRV = handler->getRV(sink);
                    const auto srcRV = handler->getRV(delayEdge->source());
                    const auto offset =
                            sink->subtype() == VertexType::OUTPUT ? srcRate * srcRV - snkRate : snkRate * snkRV;
                    lowerCons += offset;
                    upperCons += offset;
                    return computeExecDependency(delayEdge, lowerCons, upperCons, handler, std::forward<Args>(args)...);
                }

                template<class ...Args>
                i32 computeExecDependencyInput(const Edge *edge,
                                               int64_t lowerCons,
                                               int64_t upperCons,
                                               int64_t delayValue,
                                               const pisdf::GraphFiring *handler,
                                               Args &&...args) {
                    /* == Case of input interface == */
                    const auto *source = edge->source();
                    const auto srcRate = handler->getSrcRate(edge);
                    const auto *ghdl = handler->getParent()->base();
                    const auto upperLCons = srcRate * handler->firingValue();
                    const auto *upperEdge = source->graph()->inputEdge(source->ix());
                    auto dep = createExecDependency(edge, lowerCons, upperCons, srcRate, delayValue, handler);
                    i32 count = 0;
                    for (auto k = dep.firingStart_; k <= dep.firingEnd_; ++k) {
                        const auto start = k == dep.firingStart_ ? (lowerCons - delayValue) % srcRate : 0;
                        const auto end = k == dep.firingEnd_ ? (upperCons - delayValue) % srcRate : srcRate - 1;
                        count += computeExecDependency(upperEdge, upperLCons + start, upperLCons + end, ghdl,
                                                       std::forward<Args>(args)...);
                    }
                    return count;
                }

                template<class ...Args>
                i32 computeExecDependencyGraph(const Edge *edge,
                                               int64_t lowerCons,
                                               int64_t upperCons,
                                               int64_t delayValue,
                                               const pisdf::GraphFiring *handler,
                                               Args &&...args) {
                    static DependencyInfo unresolved = { nullptr, nullptr, -1,
                                                         UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX };
                    /* == Case of source graph == */
                    const auto *source = edge->source();
                    const auto srcRate = handler->getSrcRate(edge);
                    const auto *graph = source->convertTo<pisdf::Graph>();
                    const auto *innerEdge = graph->outputInterface(edge->sourcePortIx())->edge();
                    const auto ifDelay = innerEdge->delay() ? innerEdge->delay()->value() : 0u;
                    auto dep = createExecDependency(edge, lowerCons, upperCons, srcRate, delayValue, handler);
                    i32 count = 0;
                    for (auto k = dep.firingStart_; k <= dep.firingEnd_; ++k) {
                        const auto *ghdl = handler->getSubgraphGraphFiring(graph, k);
                        if (ghdl->isResolved()) {
                            const auto ifSrcRV = ghdl->getRV(innerEdge->source());
                            const auto ifSrcRate = ghdl->getSrcRate(innerEdge);
                            const auto start = k == dep.firingStart_ ? (lowerCons - delayValue) % srcRate : 0;
                            const auto end = k == dep.firingEnd_ ? (upperCons - delayValue) % srcRate : srcRate - 1;
                            const auto lCons = ifSrcRV * ifSrcRate - srcRate + start + ifDelay;
                            const auto uCons = ifSrcRV * ifSrcRate - srcRate + end + ifDelay;
                            count += computeExecDependency(innerEdge, lCons, uCons, ghdl, std::forward<Args>(args)...);
                        } else {
                            apply(unresolved, std::forward<Args>(args)...);
                        }
                    }
                    return count;
                }
            }

            template<class ...Args>
            i32 computeExecDependency(const Edge *edge,
                                      int64_t lowerCons,
                                      int64_t upperCons,
                                      const GraphFiring *handler,
                                      Args &&...args) {

                const auto *source = edge->source();
                const auto sourceType = source->subtype();
                const auto *delay = edge->delay();
                const auto delayValue = delay ? delay->value() : 0;
                if (!handler->getSnkRate(edge)) {
                    impl::apply({ nullptr, nullptr, 0, 0, 0, 0, 0, 0 }, std::forward<Args>(args)...);
                    return 0;
                }
                /* == Handle specific cases == */
                if (sourceType == VertexType::DELAY) {
                    return impl::computeExecDependencyGetter(edge, lowerCons, upperCons, handler,
                                                             std::forward<Args>(args)...);
                } else if (lowerCons >= delayValue) {
                    /* == source only == */
                    if (sourceType == VertexType::INPUT) {
                        return impl::computeExecDependencyInput(edge, lowerCons, upperCons, delayValue, handler,
                                                                std::forward<Args>(args)...);
                    } else if (sourceType == VertexType::GRAPH) {
                        return impl::computeExecDependencyGraph(edge, lowerCons, upperCons, delayValue, handler,
                                                                std::forward<Args>(args)...);
                    } else {
                        const auto srcRate = handler->getSrcRate(edge);
                        auto dep = impl::createExecDependency(edge, lowerCons, upperCons, srcRate, delayValue, handler);
                        impl::apply(dep, std::forward<Args>(args)...);
                        return static_cast<i32>(dep.firingEnd_ - dep.firingStart_ + 1);
                    }
                } else if (delay && (upperCons < delayValue)) {
                    /* == setter only == */
                    const auto *setterEdge = delay->setter()->outputEdge(delay->setterPortIx());
                    return computeExecDependency(setterEdge, lowerCons, upperCons, handler,
                                                 std::forward<Args>(args)...);
                } else if (delay) {
                    /* == setter + source == */
                    const auto *setterEdge = delay->setter()->outputEdge(delay->setterPortIx());
                    return computeExecDependency(setterEdge, lowerCons, delayValue - 1, handler,
                                                 std::forward<Args>(args)...) +
                           computeExecDependency(edge, delayValue, upperCons, handler, std::forward<Args>(args)...);
                } else {
                    throwNullptrException();
                }
            }

            /**
             * @brief Compute execution dependencies for a given INPUT edge and a given firing of the associated
             *        vertex.
             * @tparam Args    Additional parameters to be used "on site" of each dependency computation.
             *                 For now, are supported:
             *                 - functions with arbitraty number of arguments but first MUST be of type const DependencyInfo &.
             *                 - void args
             *                 - spider::vector<DependencyInfo&> &
             * @param handler  Pointer to the @refitem GraphFiring (used for rate resolution).
             * @param edge     Pointer to the edge.
             * @param firing   Value of the firing of the SINK of the edge.
             * @param args     Additionnal arguments to be passed along.
             * @return number of dependencies.
             */
            template<class ...Args>
            i32 computeExecDependency(const GraphFiring *handler, const Edge *edge, u32 firing, Args &&...args) {
                const auto snkRate = handler->getSnkRate(edge);
                return computeExecDependency(edge, snkRate * firing, snkRate * (firing + 1) - 1, handler,
                                             std::forward<Args>(args)...);
            }
        }
    }
}

#endif //SPIDER2_EXECDEPENDENCIESIMPL_H
