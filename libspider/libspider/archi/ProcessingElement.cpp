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

#include <archi/ProcessingElement.h>
#include <archi/Cluster.h>
#include <archi/Platform.h>

/* === Static variable(s) === */

static std::uint32_t spiderUniqueIx() {
    static std::uint32_t ix = 0;
    return ix++;
}

/* === Static function(s) === */

/* === Method(s) implementation === */


spider::PE::PE(std::uint32_t hwType,
               std::uint32_t hwIx,
               std::uint32_t virtIx,
               Cluster *cluster,
               std::string name,
               spider::PEType spiderPEType,
               spider::HWType spiderHWType) : hwType_{ hwType },
                                              hwIx_{ hwIx },
                                              virtIx_{ virtIx },
                                              name_{ std::move(name) },
                                              cluster_{ cluster },
                                              spiderPEIx_{ spiderUniqueIx() },
                                              spiderPEType_{ spiderPEType },
                                              spiderHWType_{ spiderHWType } {
    if (isLRT()) {
        managingLRT_ = this;
        managingLRTIx_ = cluster->platform().LRTCount();
    }
    cluster->addPE(this);
}

void spider::PE::enable() {
    enabled_ = true;
    cluster_->setPEStatus(clusterPEIx_, true);
}

void spider::PE::disable() {
    enabled_ = false;
    cluster_->setPEStatus(clusterPEIx_, false);
}

spider::MemoryUnit &spider::PE::memoryUnit() const {
    return cluster()->memoryUnit();
}
