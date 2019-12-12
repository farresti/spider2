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

#include <api/pisdf-api.h>
#include <graphs/pisdf/Vertex.h>
#include <graphs/pisdf/Graph.h>
#include <graphs/pisdf/Param.h>
#include <graphs/pisdf/Edge.h>
#include <graphs/pisdf/Delay.h>
#include <graphs/pisdf/ExecVertex.h>
#include <graphs/pisdf/SpecialVertex.h>
#include <graphs/pisdf/DynamicParam.h>
#include <graphs/pisdf/InHeritedParam.h>
#include <graphs/pisdf/interfaces/InputInterface.h>
#include <graphs/pisdf/interfaces/OutputInterface.h>

/* === Methods implementation === */

spider::pisdf::Graph *&spider::pisdfGraph() {
    static spider::pisdf::Graph *graph = nullptr;
    return graph;
}

spider::pisdf::Graph *spider::api::createGraph(std::string name,
                                               uint32_t actorCount,
                                               uint32_t edgeCount,
                                               uint32_t paramCount,
                                               uint32_t inIFCount,
                                               uint32_t outIFCount,
                                               uint32_t cfgActorCount,
                                               StackID stack) {
    return spider::make<spider::pisdf::Graph>(stack,
                                              std::move(name),
                                              actorCount,
                                              edgeCount,
                                              paramCount,
                                              inIFCount,
                                              outIFCount,
                                              cfgActorCount,
                                              stack);
}

spider::pisdf::Graph *spider::api::createSubraph(spider::pisdf::Graph *graph,
                                                 std::string name,
                                                 uint32_t actorCount,
                                                 uint32_t edgeCount,
                                                 uint32_t paramCount,
                                                 uint32_t inIFCount,
                                                 uint32_t outIFCount,
                                                 uint32_t cfgActorCount,
                                                 StackID stack) {
    auto *subgraph = spider::make<spider::pisdf::Graph>(stack,
                                                        std::move(name),
                                                        actorCount,
                                                        edgeCount,
                                                        paramCount,
                                                        inIFCount,
                                                        outIFCount,
                                                        cfgActorCount,
                                                        stack);
    graph->addVertex(subgraph);
    return subgraph;
}

spider::pisdf::ExecVertex *spider::api::createVertex(spider::pisdf::Graph *graph,
                                                     std::string name,
                                                     uint32_t edgeINCount,
                                                     uint32_t edgeOUTCount,
                                                     StackID stack) {
    auto *vertex = spider::make<spider::pisdf::ExecVertex>(stack, std::move(name), edgeINCount, edgeOUTCount, stack);
    graph->addVertex(vertex);
    return vertex;
}

spider::pisdf::ExecVertex *spider::api::createVertex(spider::pisdf::Graph *graph,
                                                     uint32_t refinementIx,
                                                     std::string name,
                                                     uint32_t edgeINCount,
                                                     uint32_t edgeOUTCount,
                                                     StackID stack) {
    auto *vertex = spider::api::createVertex(graph, std::move(name), edgeINCount, edgeOUTCount, stack);
    vertex->setRefinementIx(refinementIx);
    return vertex;
}

spider::pisdf::ExecVertex *
spider::api::createFork(spider::pisdf::Graph *graph, std::string name, uint32_t edgeOUTCount, StackID stack) {
    auto *vertex = spider::make<spider::pisdf::ForkVertex>(stack, std::move(name), edgeOUTCount, stack);
    vertex->setRefinementIx(0);
    graph->addVertex(vertex);
    return vertex;
}

spider::pisdf::ExecVertex *
spider::api::createJoin(spider::pisdf::Graph *graph, std::string name, uint32_t edgeINCount, StackID stack) {
    auto *vertex = spider::make<spider::pisdf::JoinVertex>(stack, std::move(name), edgeINCount, stack);
    vertex->setRefinementIx(1);
    graph->addVertex(vertex);
    return vertex;
}

spider::pisdf::ExecVertex *
spider::api::createHead(spider::pisdf::Graph *graph, std::string name, uint32_t edgeINCount, StackID stack) {
    auto *vertex = spider::make<spider::pisdf::HeadVertex>(stack, std::move(name), edgeINCount, stack);
    vertex->setRefinementIx(2);
    graph->addVertex(vertex);
    return vertex;
}

spider::pisdf::ExecVertex *
spider::api::createTail(spider::pisdf::Graph *graph, std::string name, uint32_t edgeINCount, StackID stack) {
    auto *vertex = spider::make<spider::pisdf::TailVertex>(stack, std::move(name), edgeINCount, stack);
    vertex->setRefinementIx(3);
    graph->addVertex(vertex);
    return vertex;
}

spider::pisdf::ExecVertex *
spider::api::createDuplicate(spider::pisdf::Graph *graph, std::string name, uint32_t edgeOUTCount, StackID stack) {
    auto *vertex = spider::make<spider::pisdf::DuplicateVertex>(stack, std::move(name), edgeOUTCount, stack);
    vertex->setRefinementIx(4);
    graph->addVertex(vertex);
    return vertex;
}

spider::pisdf::ExecVertex *spider::api::createRepeat(spider::pisdf::Graph *graph, std::string name, StackID stack) {
    auto *vertex = spider::make<spider::pisdf::RepeatVertex>(stack, std::move(name), stack);
    vertex->setRefinementIx(5);
    graph->addVertex(vertex);
    return vertex;
}

spider::pisdf::ExecVertex *spider::api::createInit(spider::pisdf::Graph *graph, std::string name, StackID stack) {
    auto *vertex = spider::make<spider::pisdf::InitVertex>(stack, std::move(name), stack);
    vertex->setRefinementIx(6);
    graph->addVertex(vertex);
    return vertex;
}

spider::pisdf::ExecVertex *spider::api::createEnd(spider::pisdf::Graph *graph, std::string name, StackID stack) {
    auto *vertex = spider::make<spider::pisdf::EndVertex>(stack, std::move(name), stack);
    vertex->setRefinementIx(7);
    graph->addVertex(vertex);
    return vertex;
}

spider::pisdf::ExecVertex *spider::api::createConfigActor(spider::pisdf::Graph *graph,
                                                          std::string name,
                                                          uint32_t edgeINCount,
                                                          uint32_t edgeOUTCount,
                                                          StackID stack) {
    auto *vertex = spider::make<spider::pisdf::ConfigVertex>(stack, std::move(name), edgeINCount, edgeOUTCount, stack);
    graph->addVertex(vertex);
    return vertex;
}

spider::pisdf::InputInterface *
spider::api::setInputInterfaceName(spider::pisdf::Graph *graph, uint32_t ix, std::string name) {
    auto *interface = graph->inputInterface(ix);
    if (!interface) {
        throwSpiderException("no input interface at index %"
                                     PRIu32
                                     " in graph [%s]", graph->name().c_str());
    }
    interface->setName(std::move(name));
    return interface;
}

spider::pisdf::OutputInterface *
spider::api::setOutputInterfaceName(spider::pisdf::Graph *graph, uint32_t ix, std::string name) {
    auto *interface = graph->outputInterface(ix);
    if (!interface) {
        throwSpiderException("no output interface at index %"
                                     PRIu32
                                     " in graph [%s]", graph->name().c_str());
    }
    interface->setName(std::move(name));
    return interface;
}

/* === Param creation API === */

spider::pisdf::Param *
spider::api::createStaticParam(spider::pisdf::Graph *graph, std::string name, int64_t value, StackID stack) {
    auto *param = spider::make<spider::pisdf::Param>(stack, std::move(name), value);
    if (graph) {
        graph->addParam(param);
    }
    return param;
}

spider::pisdf::Param *
spider::api::createStaticParam(spider::pisdf::Graph *graph, std::string name, std::string expression, StackID stack) {
    if (graph) {
        auto *param = spider::make<spider::pisdf::Param>(stack, std::move(name),
                                                         Expression(std::move(expression), graph->params()));
        graph->addParam(param);
        return param;
    }
    auto *param = spider::make<spider::pisdf::Param>(stack, std::move(name), Expression(std::move(expression)));
    return param;
}

spider::pisdf::DynamicParam *
spider::api::createDynamicParam(spider::pisdf::Graph *graph, std::string name, StackID stack) {
    auto *param = spider::make<spider::pisdf::DynamicParam>(stack, std::move(name), Expression(0));
    if (graph) {
        graph->addParam(param);
    }
    return param;
}

spider::pisdf::DynamicParam *
spider::api::createDynamicParam(spider::pisdf::Graph *graph, std::string name, std::string expression, StackID stack) {
    if (graph) {
        auto *param = spider::make<spider::pisdf::DynamicParam>(stack, std::move(name),
                                                                Expression(std::move(expression), graph->params()));
        graph->addParam(param);
        return param;
    }
    auto *param = spider::make<spider::pisdf::DynamicParam>(stack, std::move(name), Expression(std::move(expression)));
    return param;
}

spider::pisdf::Param *
spider::api::createInheritedParam(spider::pisdf::Graph *graph, std::string name, spider::pisdf::Param *parent,
                                  StackID stack) {
    if (!parent) {
        throwSpiderException("Cannot instantiate inherited parameter with null parent.");
    }
    if (!parent->dynamic()) {
        return spider::api::createStaticParam(graph, std::move(name), parent->value(), stack);
    }
    auto *param = spider::make<spider::pisdf::InHeritedParam>(stack, std::move(name), parent);
    if (graph) {
        graph->addParam(param);
    }
    return param;
}

/* === Edge API === */

spider::pisdf::Edge *spider::api::createEdge(spider::pisdf::Vertex *source,
                                             size_t srcPortIx,
                                             std::string srcRateExpression,
                                             spider::pisdf::Vertex *sink,
                                             size_t snkPortIx,
                                             std::string snkRateExpression,
                                             StackID stack) {
    auto *edge = spider::make<spider::pisdf::Edge>(stack,
                                                   source,
                                                   srcPortIx,
                                                   Expression(std::move(srcRateExpression), source->graph()->params()),
                                                   sink,
                                                   snkPortIx,
                                                   Expression(std::move(snkRateExpression), sink->graph()->params()));
    source->graph()->addEdge(edge);
    return edge;
}

spider::pisdf::Edge *spider::api::createEdge(spider::pisdf::Vertex *source,
                                             size_t srcPortIx,
                                             int64_t srcRate,
                                             spider::pisdf::Vertex *sink,
                                             size_t snkPortIx,
                                             int64_t snkRate,
                                             StackID stack) {
    auto *edge = spider::make<spider::pisdf::Edge>(stack,
                                                   source,
                                                   srcPortIx,
                                                   Expression(srcRate),
                                                   sink,
                                                   snkPortIx,
                                                   Expression(snkRate));
    source->graph()->addEdge(edge);
    return edge;
}

spider::pisdf::Delay *spider::api::createDelay(spider::pisdf::Edge *edge,
                                               std::string delayExpression,
                                               spider::pisdf::ExecVertex *setter,
                                               uint32_t setterPortIx,
                                               const std::string &setterRateExpression,
                                               spider::pisdf::ExecVertex *getter,
                                               uint32_t getterPortIx,
                                               const std::string &getterRateExpression,
                                               bool persistent,
                                               StackID stack) {
    if (delayExpression == "0" && log_enabled()) {
        spider::log::warning("delay with null value on edge [%s] ignored.\n",
                             edge->name().c_str());
        return nullptr;
    }
    const auto expression = delayExpression;
    auto *delay = spider::make<spider::pisdf::Delay>(stack,
                                                     Expression(std::move(delayExpression), edge->graph()->params()),
                                                     edge,
                                                     setter,
                                                     setterPortIx,
                                                     Expression(setter ? setterRateExpression : expression,
                                                                edge->graph()->params()),
                                                     getter,
                                                     getterPortIx,
                                                     Expression(getter ? getterRateExpression : expression,
                                                                edge->graph()->params()),
                                                     persistent);
    return delay;
}

spider::pisdf::Delay *spider::api::createDelay(spider::pisdf::Edge *edge,
                                               int64_t value,
                                               spider::pisdf::ExecVertex *setter,
                                               uint32_t setterPortIx,
                                               int64_t setterRate,
                                               spider::pisdf::ExecVertex *getter,
                                               uint32_t getterPortIx,
                                               int64_t getterRate,
                                               bool persistent,
                                               StackID stack) {
//    if (!value && log_enabled()) {
//        spider::log::warning("delay with null value on edge [%s] ignored.\n",
//                             edge->name().c_str());
//        return nullptr;
//    }
    return spider::make<spider::pisdf::Delay>(stack,
                                              Expression(value),
                                              edge,
                                              setter,
                                              setterPortIx,
                                              Expression(setter ? setterRate : value),
                                              getter,
                                              getterPortIx,
                                              Expression(getter ? getterRate : value),
                                              persistent);
}