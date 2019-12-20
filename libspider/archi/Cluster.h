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
#ifndef SPIDER2_CLUSTER_H
#define SPIDER2_CLUSTER_H

/* === Include(s) === */

#include <cstdint>
#include <algorithm>
#include <containers/array.h>
#include <archi/PE.h>
#include <api/archi-api.h>

namespace spider {

    /* === Class definition === */

    class Cluster {
    public:

        Cluster(size_t PECount, MemoryUnit *memoryUnit, MemoryInterface *memoryInterface);

        ~Cluster();

        /* === Method(s) === */

        /**
         * @brief Add a processing element to the cluster.
         * @param PE Processing element to add.
         * @throws @refitem std::out_of_range if cluster is already full.
         */
        void addPE(PE *pe);

        /**
         * @brief Set the state (enabled or disabled) of a processing element in the cluster.
         * @param ix      Ix of the processing element.
         * @param status  Status of the PE to set (true = enabled, false = disabled).
         * @throws std::out_of_range if PE ix is out of bound.
         */
        inline void setPEStatus(uint32_t ix, bool status) {
            status ? PEArray_.at(ix)->enable() : PEArray_.at(ix)->disable();
        }

        /* === Getter(s) === */

        /**
         * @brief Get the array of processing element of the cluster.
         * @return const reference to the @refitem spider::array of @refitem PE of the cluster.
         */
        inline const spider::array<PE *> &array() const {
            return PEArray_;
        }

        /**
         * @brief Get the memory unit of the cluster.
         * @return pointer to the @refitem MemoryUnit of the cluster.
         */
        inline MemoryUnit *memoryUnit() const {
            return memoryUnit_;
        }

        /**
         * @brief Get the memory interface of the cluster.
         * @return pointer to the @refitem MemoryInterface of the cluster.
         */
        inline MemoryInterface *memoryInterface() const {
            return memoryInterface_;
        }

        /**
         * @brief Get a given processing element from the cluster.
         * @param ix  Ix of the processing element in the cluster.
         * @return const reference of the @refitem ProcessingElement
         * @throws @refitem std::out_of_range if ix is out of bound
         */
        inline PE *at(size_t ix) const {
            return PEArray_.at(ix);
        }

        /**
         * @brief Get the number of processing element actually inside the cluster.
         * @return number of @refitem PE inside the cluster.
         */
        inline size_t PECount() const {
            return PECount_;
        }

        /**
         * @brief Get the number of local runtime in the cluster.
         * @return number of local runtime inside the cluster.
         */
        inline size_t LRTCount() const {
            return LRTCount_;
        }

        /**
         * @brief Get the PE type of the cluster.
         * @remark This method return the value of @refitem PE::hardwareType() method of the first PE.
         * @return PE type of the cluster.
         */
        inline uint32_t PEType() const {
            return PEArray_[0]->hardwareType();
        }

        /**
         * @brief  Get the cluster ix (unique among clusters).
         * @return Ix of the cluster.
         */
        inline size_t ix() const {
            return ix_;
        }

        /**
         * @brief Get the platform of the cluster.
         * @return @refitem Platform of the cluster.
         */
        inline Platform *platform() const {
            return archi::platform();
        }

        /* === Setter(s) === */

        /**
         * @brief Set the cluster ix inside the Platform.
         * @param ix Ix to set.
         */
        inline void setIx(size_t ix) {
            ix_ = ix;
        }

    private:

        /* === Core properties === */

        spider::array<PE *> PEArray_;                /* = Array of PE contained in the Cluster = */
        MemoryUnit *memoryUnit_ = nullptr;           /* = Pointer to the MemoryUni associated to the Cluster = */
        MemoryInterface *memoryInterface_ = nullptr; /* = Pointer to the MemoryInterface for intra Cluster communications = */

        /* === Spider properties === */

        size_t LRTCount_ = 0; /* = Number of Local Runtime inside this Cluster = */
        size_t PECount_ = 0;  /* = Number of currently added PE in the Cluster */
        size_t ix_ = 0;       /* = Linear index of the Cluster in the Platform */

    };
}
#endif //SPIDER2_CLUSTER_H
