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
#ifndef SPIDER2_VECTPISDFTASK_H
#define SPIDER2_VECTPISDFTASK_H

/* === Include(s) === */

#include <scheduling/task/PiSDFTask.h>
#include <memory/unique_ptr.h>
#include <graphs-tools/numerical/detail/DependencyIterator.h>

namespace spider {

    namespace pisdf {
        class Vertex;

        class Edge;

        class GraphFiring;
    }

    namespace sched {

        /* === Class definition === */

        class VectPiSDFTask final : public PiSDFTask {
        public:
            VectPiSDFTask(pisdf::GraphFiring *handler, const pisdf::Vertex *vertex);

            ~VectPiSDFTask() final = default;

            /* === Method(s) === */

            void reset() final;

            /* === Getter(s) === */

            u64 endTime() const final;

            const PE *mappedPe() const final;

            TaskState state() const noexcept final;

            u32 jobExecIx() const noexcept final;

            u32 syncExecIxOnLRT(size_t lrtIx) const final;

            u32 syncRateOnLRT(size_t lrtIx) const final;

            /* === Setter(s) === */

            void setOnFiring(u32 firing) override;

            void setEndTime(u64 time) final;

            void setMappedPE(const PE *pe) final;

            void setState(TaskState state) noexcept final;

            void setJobExecIx(u32 ix) noexcept final;

            void setSyncExecIxOnLRT(size_t lrtIx, u32 value) final;

            void setSyncRateOnLRT(size_t lrtIx, u32 value) final;

        private:
            spider::unique_ptr<SyncInfo> syncInfoArray_; /*!< Exec constraints array of the instances of the vertex*/
            spider::unique_ptr<u64> endTimeArray_;       /*!< Mapping end time array of the instances of the vertex */
            spider::unique_ptr<u32> mappedPEIxArray_;    /*!< Mapping PE array of the instances of the vertex */
            spider::unique_ptr<u32> jobExecIxArray_;     /*!< Index array of the job sent to the PE */
            u32 currentOffset_ = 0;
            spider::unique_ptr<TaskState> stateArray_;   /*!< State array of the instances of the vertex */
        };
    }
}

#endif //SPIDER2_VECTPISDFTASK_H
