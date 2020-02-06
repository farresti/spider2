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

/* === Include(s) === */

#include <graphs/pisdf/Graph.h>
#include <graphs-tools/helper/visitors/PiSDFDefaultVisitor.h>
#include <graphs/pisdf/ExecVertex.h>
#include <graphs/pisdf/interfaces/Interface.h>
#include <graphs/pisdf/interfaces/InputInterface.h>
#include <graphs/pisdf/interfaces/OutputInterface.h>
#include <graphs/pisdf/Param.h>
#include <graphs/pisdf/Delay.h>

/* === Private structure(s) === */

struct spider::pisdf::Graph::RemoveSpecialVertexVisitor final : public DefaultVisitor {

    explicit RemoveSpecialVertexVisitor(Graph *graph) : graph_{ graph } { };

    /* === Method(s) === */

    inline void visit(Graph *subgraph) override {
        /* == Remove the vertex and destroy it == */
        auto ix = subgraph->subIx_; /* = Save the index in the subgraphVector_ = */

        /* == Remove the subgraph from the subgraph vector == */
        graph_->subgraphVector_[ix] = graph_->subgraphVector_.back();
        graph_->subgraphVector_[ix]->subIx_ = ix;
        graph_->subgraphVector_.pop_back();
    }

private:
    Graph *graph_ = nullptr;
};

struct spider::pisdf::Graph::AddSpecialVertexVisitor final : public DefaultVisitor {

    explicit AddSpecialVertexVisitor(Graph *graph) : graph_{ graph } { }

    inline void visit(Graph *subgraph) override {
        /* == Add the subgraph in the "viewer" vector == */
        subgraph->subIx_ = graph_->subgraphVector_.size();
        graph_->subgraphVector_.emplace_back(subgraph);
    }

private:
    Graph *graph_ = nullptr;
};

/* === Method(s) implementation === */

spider::pisdf::Graph::Graph(std::string name,
                            size_t vertexCount,
                            size_t edgeCount,
                            size_t paramCount,
                            size_t edgeINCount,
                            size_t edgeOUTCount,
                            size_t cfgVertexCount) :
        Vertex(VertexType::GRAPH, std::move(name), edgeINCount, edgeOUTCount),
        vertexVector_{ sbc::vector < unique_ptr < Vertex > , StackID::PISDF > { }},
        edgeVector_{ sbc::vector < unique_ptr < Edge > , StackID::PISDF > { }},
        configVertexVector_{ sbc::vector < Vertex * , StackID::PISDF > { }},
        subgraphVector_{ sbc::vector < Graph * , StackID::PISDF > { }},
        paramVector_{ sbc::vector < std::shared_ptr<Param>, StackID::PISDF > { }},
        inputInterfaceVector_{ sbc::vector < unique_ptr < InputInterface > , StackID::PISDF > { }},
        outputInterfaceVector_{ sbc::vector < unique_ptr < OutputInterface > , StackID::PISDF > { }} {

    /* == Reserve the memory == */
    vertexVector_.reserve(vertexCount);
    edgeVector_.reserve(edgeCount);
    paramVector_.reserve(paramCount);
    configVertexVector_.reserve(cfgVertexCount);
    inputInterfaceVector_.reserve(edgeINCount);
    outputInterfaceVector_.reserve(edgeOUTCount);

    /* == Create the input interfaces == */
    for (size_t i = 0; i < edgeINCount; ++i) {
        auto *interface = make<InputInterface, StackID::PISDF>("in_" + std::to_string(i));
        addInputInterface(interface);
    }

    /* == Create the output interfaces == */
    for (size_t i = 0; i < edgeOUTCount; ++i) {
        auto *interface = make<OutputInterface, StackID::PISDF>("out_" + std::to_string(i));
        addOutputInterface(interface);
    }
}

void spider::pisdf::Graph::clear() {
    edgeVector_.clear();
    vertexVector_.clear();
    paramVector_.clear();
    subgraphVector_.clear();
    configVertexVector_.clear();
}

void spider::pisdf::Graph::addInputInterface(InputInterface *interface) {
    if (!interface) {
        return;
    }
    /* == Adds the interface to the graph == */
    interface->setIx(inputInterfaceVector_.size());
    interface->setGraph(this);
    inputInterfaceVector_.emplace_back(interface);

    /* == Resize the input edge vector == */
    if (inputEdgeCount() < inputInterfaceVector_.size()) {
        inputEdgeVector_.emplace_back(nullptr);
    }
}

void spider::pisdf::Graph::addOutputInterface(OutputInterface *interface) {
    if (!interface) {
        return;
    }
    /* == Adds the interface to the graph == */
    interface->setIx(outputInterfaceVector_.size());
    interface->setGraph(this);
    outputInterfaceVector_.emplace_back(interface);

    /* == Resize the output edge vector == */
    if (outputEdgeCount() < outputInterfaceVector_.size()) {
        outputEdgeVector_.emplace_back(nullptr);
    }
}

void spider::pisdf::Graph::addVertex(Vertex *vertex) {
    if (vertex->subtype() == VertexType::CONFIG) {
        /* == Add config vertex to the "viewer" vector == */
        graph_->configVertexVector_.emplace_back(vertex);
    } else if (vertex->hierarchical()) {
        AddSpecialVertexVisitor visitor{ this };
        vertex->visit(&visitor);
    }
    vertex->setIx(vertexVector_.size());
    vertex->setGraph(this);
    vertexVector_.emplace_back(vertex);
}

void spider::pisdf::Graph::removeVertex(Vertex *vertex) {
    if (!vertex) {
        return;
    }
    /* == If got any Edges left disconnect them == */
    for (auto &edge : inputEdgeVector_) {
        edge->setSink(nullptr, SIZE_MAX, Expression());
    }
    for (auto &edge : outputEdgeVector_) {
        edge->setSource(nullptr, SIZE_MAX, Expression());
    }
    if (vertex->subtype() == VertexType::CONFIG) {
        /* == configVertexVector_ is just a "viewer" for config vertices so we need to find manually == */
        for (auto &cfg : graph_->configVertexVector_) {
            if (cfg == vertex) {
                cfg = graph_->configVertexVector_.back();
                graph_->configVertexVector_.pop_back();
                break;
            }
        }
    } else if (vertex->hierarchical()) {
        RemoveSpecialVertexVisitor visitor{ this };
        vertex->visit(&visitor);
    }
    removeAndDestroy(vertexVector_, vertex);
}

void spider::pisdf::Graph::addEdge(Edge *edge) {
    edge->setIx(edgeVector_.size());
    edgeVector_.emplace_back(edge);
}

void spider::pisdf::Graph::removeEdge(Edge *edge) {
    edge->setSource(nullptr, SIZE_MAX, Expression());
    edge->setSink(nullptr, SIZE_MAX, Expression());
    removeAndDestroy(edgeVector_, edge);
}

void spider::pisdf::Graph::addParam(std::shared_ptr<Param> param) {
    /* == Check if a parameter with the same name already exists in the scope of this graph == */
    for (auto &p : paramVector_) {
        if (p->name() == param->name()) {
            throwSpiderException("Parameter [%s] already exist in graph [%s].", param->name().c_str(), name().c_str());
        }
    }
    if (!param->graph()) {
        param->setIx(paramVector_.size());
        param->setGraph(this);
    }
    dynamic_ |= (param->dynamic() && param->type() != ParamType::INHERITED);
    paramVector_.emplace_back(std::move(param));
}

void spider::pisdf::Graph::moveVertex(Vertex *elt, Graph *graph) {
    if (!graph || (graph == this) || !elt) {
        return;
    }
    if (elt->subtype() == VertexType::CONFIG) {
        RemoveSpecialVertexVisitor visitor{ this };
        elt->visit(&visitor);
    } else if (elt->hierarchical()) {
        RemoveSpecialVertexVisitor visitor{ this };
        elt->visit(&visitor);
    }
    removeNoDestroy(vertexVector_, elt);
    graph->addVertex(elt);
}

void spider::pisdf::Graph::moveEdge(Edge *elt, Graph *graph) {
    if (!graph || (graph == this) || !elt) {
        return;
    }
    removeNoDestroy(edgeVector_, elt);
    graph->addEdge(elt);
}

spider::pisdf::Param *spider::pisdf::Graph::paramFromName(const std::string &name) {
    std::string lowerCaseName = name;
    std::transform(lowerCaseName.begin(), lowerCaseName.end(), lowerCaseName.begin(), ::tolower);
    for (auto &param : paramVector_) {
        if (param->name() == lowerCaseName) {
            return param.get();
        }
    }
    return nullptr;
}

bool spider::pisdf::Graph::setRunGraphReference(const spider::pisdf::Graph *runGraph) {
    if (dynamic() || !configVertexCount() || runGraphReference_ || !runGraph) {
        return false;
    }
    runGraphReference_ = runGraph;
    return true;
}

void spider::pisdf::Graph::overrideDynamicProperty(bool value) {
    dynamic_ = value;
}

/* === Private method(s) === */

template<class T>
void spider::pisdf::Graph::removeNoDestroy(spider::vector<unique_ptr<T>> &eltVector, T *elt) {
    auto ix = elt->ix();
    if (ix >= eltVector.size()) {
        throwSpiderException("Trying to remove an element not from this graph.");
    }
    if (eltVector[ix].get() != elt) {
        throwSpiderException("Different element in ix position. Expected: %s -- Got: %s", elt->name().c_str(),
                             eltVector[ix]->name().c_str());
    }
    if (ix == (eltVector.size() - 1)) {
        (void) eltVector.back().release();
    } else {
        (void) eltVector[ix].release();
        eltVector[ix] = std::move(eltVector.back());
        eltVector[ix]->setIx(ix);
    }
    eltVector.pop_back();
}

template<class T>
void spider::pisdf::Graph::removeAndDestroy(spider::vector<unique_ptr<T>> &eltVector, T *elt) {
    auto ix = elt->ix();
    if (ix >= eltVector.size()) {
        throwSpiderException("Trying to remove an element not from this graph.");
    }
    if (eltVector[ix].get() != elt) {
        throwSpiderException("Different element in ix position. Expected: %s -- Got: %s", elt->name().c_str(),
                             eltVector[ix]->name().c_str());
    }
    if (ix != (eltVector.size() - 1)) {
        eltVector[ix] = std::move(eltVector.back());
        eltVector[ix]->setIx(ix);
    }
    eltVector.pop_back();
}

template<class T>
void spider::pisdf::Graph::removeNoDestroy(spider::vector<T *> &eltVector, T *elt) {
    if (!elt) {
        return;
    }
    auto ix = elt->ix();
    if (ix >= eltVector.size()) {
        throwSpiderException("Trying to remove an element not from this graph.");
    }
    if (eltVector[ix] != elt) {
        throwSpiderException("Different element in ix position. Expected: %s -- Got: %s", elt->name().c_str(),
                             eltVector[ix]->name().c_str());
    }
    eltVector[ix] = std::move(eltVector.back());
    eltVector[ix]->setIx(ix);
    eltVector.pop_back();
}
