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
#ifndef SPIDER2_ARCHI_API_H
#define SPIDER2_ARCHI_API_H

/* === Includes === */

#include <cstdint>
#include <string>
#include <api/global-api.h>

namespace spider {

    inline uint64_t defaultC2CZeroCommunicationCost(uint32_t, uint32_t, uint64_t) {
        return 0;
    }

    inline uint64_t defaultZeroCommunicationCost(uint64_t) {
        return 0;
    }

    inline uint64_t defaultInfiniteCommunicationCost(uint64_t) {
        return UINT64_MAX;
    }

    /* === Function(s) prototype === */

    /**
     * @brief Get the unique platform of the Spider session.
     * @return reference pointer to the platform.
     */
    Platform *&platform();

    namespace api {

        /* === General Platform related API === */

        /**
         * @brief Create a new Platform (only one is permitted)
         * @param clusterCount   Number of cluster in the platform (1 by default)
         * @return pointer to the newly created @refitem Platform
         * @throws @refitem Spider::Exception if a platform already exists.
         */
        Platform *createPlatform(uint32_t clusterCount = 1);

        /**
         * @brief Set the Global Run-Time (GRT) PE.
         * @param grtPE  Processing Element of the GRT.
         */
        void setSpiderGRTPE(PE *grtPE);

        /**
         * @brief Set the routine that will handle the cluster to cluster communication cost.
         * @param routine  Routine to set.
         */
        void setCluster2ClusterCommunicationCostRoutine(CommunicationCostRoutineC2C routine);

        /* === Cluster related API === */

        /**
         * @brief Create a new Cluster. A cluster is a set of PE connected to a same memory unit.
         * @param PECount      Number of PE in the cluster.
         * @param memoryUnit   Memory unit of the cluster.
         * @return pointer to the newly created @refitem Cluster.
         */
        Cluster *createCluster(uint32_t PECount, MemoryUnit *memoryUnit);

        /**
         * @brief Set the write cost method for writing into cluster memory.
         * @param cluster Cluster to which the routine is set.
         * @param routine Routine to set.
         */
        void setClusterWriteCostRoutine(Cluster *cluster, CommunicationCostRoutine routine);

        /**
         * @brief Set the read cost method for writing into cluster memory.
         * @param cluster Cluster to which the routine is set.
         * @param routine Routine to set.
         */
        void setClusterReadCostRoutine(Cluster *cluster, CommunicationCostRoutine routine);

        /* === PE related API === */

        /**
         * @brief Create a new Processing Element (PE).
         * @param hwType        S-LAM user defined hardware type.
         * @param hwID          Physical hardware id of the PE (mainly used for thread affinity).
         * @param virtID        S-LAM used defined PE id.
         * @param cluster       Cluster of the PE.
         * @param name          Name of the PE.
         * @param spiderPEType  Spider PE type.
         * @param spiderHWType  Spider hardware type.
         * @return Pointer to newly created @refitem ProcessingElement, associated memory is handled by spider.
         */
        PE *createPE(uint32_t hwType,
                     uint32_t hwID,
                     uint32_t virtID,
                     Cluster *cluster,
                     const std::string &name,
                     PEType spiderPEType = PEType::LRT_PE,
                     HWType spiderHWType = HWType::PHYS_PE);

        /**
         * @brief Set the SpiderPEType of a given PE.
         * @param PE    Pointer to the PE.
         * @param type  Spider::PEType to set.
         */
        void setPESpiderPEType(PE *PE, PEType type);

        /**
         * @brief Set the SpiderHWType of a given PE.
         * @param PE    Pointer to the PE.
         * @param type  Spider::HWType to set.
         */
        void setPESpiderHWType(PE *PE, HWType type);

        /**
         * @brief Set the name of a given PE.
         * @param PE    Pointer to the PE.
         * @param name  Name of the PE to set.
         */
        void setPEName(PE *PE, const std::string &name);

        /**
         * @brief Enable a given PE (default).
         * @param PE  Pointer to the PE.
         */
        void enablePE(PE *PE);

        /**
         * @brief Disable a given PE.
         * @param PE  Pointer to the PE.
         */
        void disablePE(PE *PE);

        /* === MemoryUnit related API === */

        /**
         * @brief Create a new MemoryUnit.
         * @param base  Base address of the MemoryUnit.
         * @param size  Size of the MemoryUnit.
         * @return Pointer to newly created @refitem MemoryUnit, associated memory is handled by spider.
         */
        MemoryUnit *createMemoryUnit(void *base, uint64_t size);
    }
}

#endif //SPIDER2_ARCHI_API_H
