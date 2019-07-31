/*
 * Copyright or © or Copr. IETR/INSA - Rennes (2013 - 2018) :
 *
 * Antoine Morvan <antoine.morvan@insa-rennes.fr> (2018)
 * Clément Guy <clement.guy@insa-rennes.fr> (2014)
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2018)
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

#include "PiSDFDelay.h"
#include <graphs/pisdf/PiSDFVertex.h>
#include <graphs/pisdf/PiSDFEdge.h>
#include <spider-api/pisdf.h>

/* === Static variable(s) === */

/* === Static function(s) === */

/* === Method(s) implementation === */

PiSDFDelay::PiSDFDelay(PiSDFEdge *edge,
                       const std::string &expression,
                       bool persistent,
                       PiSDFVertex *setter,
                       PiSDFVertex *getter,
                       std::uint32_t setterPortIx,
                       std::uint32_t getterPortIx) : edge_{edge},
                                                     setter_{setter},
                                                     getter_{getter},
                                                     setterPortIx_{setterPortIx},
                                                     getterPortIx_{getterPortIx},
                                                     expression_{edge->containingGraph(), expression},
                                                     persistent_{persistent} {
    edge->setDelay(this);

    /* == Check the persistent property == */
    checkPersistence();

    /* == If delay has setter / getter, creates the virtual actor == */
    createVirtualVertex();
}

PiSDFDelay::PiSDFDelay(PiSDFEdge *edge,
                       std::int64_t value,
                       bool persistent,
                       PiSDFVertex *setter,
                       PiSDFVertex *getter,
                       std::uint32_t setterPortIx,
                       std::uint32_t getterPortIx) : edge_{edge},
                                                     setter_{setter},
                                                     getter_{getter},
                                                     setterPortIx_{setterPortIx},
                                                     getterPortIx_{getterPortIx},
                                                     expression_{value},
                                                     persistent_{persistent} {
    edge->setDelay(this);

    /* == Check the persistent property == */
    checkPersistence();

    /* == If delay has setter / getter, creates the virtual actor == */
    createVirtualVertex();
}

/* === Pritvate method(s) === */

void PiSDFDelay::checkPersistence() const {
    if (persistent_ && (setter_ || getter_)) {
        throwSpiderException("Persistent delay on edge [%s] can not have setter nor getter.", edge_->name().c_str());
    }
}

void PiSDFDelay::createVirtualVertex() {
    if (setter_ || getter_) {
        /* == Create the virtual delay actor == */
        virtualVertex_ = Spider::API::createVertex(edge_->containingGraph(),
                                                   name(),
                                                   1,
                                                   1);

        /* == If setter_ is null, replace it with init == */
        if (!setter_) {
            setter_ = Spider::API::createInit(edge_->containingGraph(), "init-" + name(), 0);
            setterPortIx_ = 0;
        }

        /* == If getter_ is null, replace it with end == */
        if (!getter_) {
            getter_ = Spider::API::createEnd(edge_->containingGraph(), "end-" + name(), 0);
            getterPortIx_ = 0;
        }

        /* == Connect setter to the virtual delay actor == */
        Spider::API::createEdge(edge_->containingGraph(),
                                setter_, setterPortIx_, expression_.toString(),
                                virtualVertex_, 0, expression_.toString());

        /* == Connect virtual delay actor to getter == */
        Spider::API::createEdge(edge_->containingGraph(),
                                virtualVertex_, 0, expression_.toString(),
                                getter_, getterPortIx_, expression_.toString());
    }
}

std::string PiSDFDelay::name() const {
    return "delay-" + edge_->source()->name() + "--" + edge_->sink()->name();
}
