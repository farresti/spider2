/*
 * Copyright or © or Copr. IETR/INSA - Rennes (2020) :
 *
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2020)
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
#ifndef SPIDER2_FIFOALLOCATOR_H
#define SPIDER2_FIFOALLOCATOR_H

/* === Include(s) === */

#include <runtime/common/Fifo.h>
#include <scheduling/memory/AllocationRule.h>
#include <api/global-api.h>

namespace spider {

    /* === Forward declaration(s) === */

    class MemoryInterface;

    namespace pisdf {
        class Graph;
    }

    namespace sched {

        class Task;

        class SRLessTask;

        class VertexTask;

        class SyncTask;

        /* === Class definition === */

        class FifoAllocator {
        public:
            struct FifoAllocatorTraits {
                bool jitAllocator_;
                bool postSchedulingAllocator_;
            };

            /* === Allocator traits === */

            FifoAllocatorTraits traits_;

            FifoAllocator() : FifoAllocator({ true, true }) { }

            virtual ~FifoAllocator() noexcept = default;

            /* === Method(s) === */

            /**
             * @brief Allocate Fifos of a given task.
             * @param task Pointer to the task.
             */
            virtual void allocate(sched::SyncTask *task);

            /**
             * @brief Allocate Fifos of a given task.
             * @param task Pointer to the task.
             */
            inline virtual void allocate(sched::SRLessTask *) {}

            /**
             * @brief Allocate Fifos of a given task.
             * @param task Pointer to the task.
             */
            virtual void allocate(sched::VertexTask *task);

            /**
             * @brief Clears the allocator.
             */
            virtual void clear() noexcept;

            /**
             * @brief Reserve memory for permanent delays.
             * @param graph pointer to the graph.
             */
            virtual void allocatePersistentDelays(pisdf::Graph *graph);

            /* === Getter(s) === */

            /**
             * @brief Get the type of the FifoAllocator
             * @return @refitem FifoAllocatorType
             */
            virtual FifoAllocatorType type() const { return FifoAllocatorType::DEFAULT; };

        protected:
            size_t reservedMemory_ = 0;
            size_t virtualMemoryAddress_ = 0;

            explicit FifoAllocator(FifoAllocatorTraits traits) noexcept: traits_{ traits } { }
        };
    }
}
#endif //SPIDER2_FIFOALLOCATOR_H
