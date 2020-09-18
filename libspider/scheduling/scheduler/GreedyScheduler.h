/**
 * Copyright or © or Copr. IETR/INSA - Rennes (2019 - 2020) :
 *
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2019 - 2020)
 *
 * Spider 2.0 is a dataflow based runtime used to execute dynamic PiSDF
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
#ifndef SPIDER2_GREEDYSCHEDULER_H
#define SPIDER2_GREEDYSCHEDULER_H

/* === Include(s) === */

#include <scheduling/scheduler/Scheduler.h>

namespace spider {

    namespace sched {

        /* === Class definition === */

        class GreedyScheduler final : public Scheduler {
        public:

            GreedyScheduler() : Scheduler() { };

            ~GreedyScheduler() noexcept override = default;

            /* === Method(s) === */

            void schedule(const pisdf::Graph *graph) override;

            void clear() override;

            /* === Getter(s) === */

            /* === Setter(s) === */

            /* === Getter(s) === */

            /* === Setter(s) === */

        private:

            /* === Types definition === */

            struct ScheduleVertex {
                pisdf::Vertex *vertex_;
                bool executable_;
            };
            using iterator_t = spider::vector<ScheduleVertex>::iterator;

            /* === Private member(s) === */

            spider::vector<ScheduleVertex> unscheduledVertices_;

            /* === Private method(s) === */

            /**
             * @brief Reset unscheduled task from previous schedule iteration.
             */
            void resetUnScheduledTasks();

            /**
             * @brief Recursively sets all output of a given vertex as non executable.
             * @param scheduleVertex Reference to current @refitem GreedyScheduler::ScheduleVertex.
             */
            void setSinksAsNonExecutable(ScheduleVertex &scheduleVertex);

            /**
             * @brief Evaluate current vertex pointed by the iterator it for schedulability.
             * @param it Iterator to the vertex to be evaluated.
             * @return it + 1 if vertex was schedulable due to dependency not being executable,
             *         iterator to one of the source of the vertex if it was schedulable due to dependency not yet satisfied,
             *         iterator to last value of the unscheduledVertices_ vector if vertex was scheduled.
             */
            iterator_t evaluate(iterator_t it);

            /**
             * @brief Remove value at given position and swap it with the value at the end of the unscheduledVertices_ vector.
             * @param it Iterator to the value to be removed.
             * @return iterator pointing to the element at the same position as the removed one (end iterator else)
             */
            iterator_t removeAndSwap(iterator_t it);

            /**
             * @brief Remove all non executable (!= non schedulable) vertices from the  unscheduledVertices_ vector.
             */
            void removeNonExecutableVertices();
        };
    }
}

#endif //SPIDER2_GREEDYSCHEDULER_H
