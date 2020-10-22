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
#ifndef SPIDER2_TASK_H
#define SPIDER2_TASK_H

/* === Include(s) === */

#include <memory/memory.h>
#include <common/Types.h>
#include <containers/array.h>
#include <scheduling/memory/JobFifos.h>
#include <scheduling/memory/AllocationRule.h>
#include <runtime/message/JobMessage.h>

namespace spider {

    /* === Forward Declaration(s) === */

    class PE;

    namespace sched {

        class Task;

        class Schedule;

        class FifoAllocator;

        struct DependencyInfo {
            size_t fifoIx_;
            size_t dataSize_;
        };

        enum class TaskState : u8 {
            NOT_SCHEDULABLE = 0,
            NOT_RUNNABLE,
            PENDING,
            READY,
            RUNNING,
        };

        /* === Class definition === */

        class Task {
        public:
            Task();

            virtual ~Task() noexcept = default;

            Task(Task &&) noexcept = default;

            Task &operator=(Task &&) noexcept = default;

            Task(const Task &) = delete;

            Task &operator=(const Task &) noexcept = delete;

            /* === Method(s) === */

            /**
             * @brief Set all notification flags to true.
             */
            void enableBroadcast();

            spider::array<SyncInfo> getExecutionConstraints() const;

            /* === Getter(s) === */

            inline JobFifos &fifos() const {
#ifndef NDEBUG
                if (!fifos_) {
                    throwSpiderException("Nullptr TaskFifos.");
                }
#endif
                return *(fifos_.get());
            }

            /**
             * @brief Get the start time of the task.
             * @return mapping start time of the task, UINT64_MAX else
             */
            u64 startTime() const;

            /**
             * @brief Get the end time of the task.
             * @return mapping end time of the task, UINT64_MAX else
             */
            u64 endTime() const;

            /**
             * @brief Returns the PE on which the task is mapped.
             * @return pointer to the PE onto which the task is mapped, nullptr else
             */
            const PE *mappedPe() const;

            /**
             * @brief Returns the LRT attached to the PE on which the task is mapped.
             * @return pointer to the LRT, nullptr else
             */
            const PE *mappedLRT() const;

            /**
             * @brief Returns the state of the task.
             * @return @refitem TaskState of the task
             */
            inline TaskState state() const noexcept { return state_; }

            /**
             * @brief Returns the ix of the task in the schedule.
             * @return ix of the task in the schedule, -1 else.
             */
            inline u32 ix() const noexcept { return ix_; }

            /**
             * @brief Returns the executable job index value of the task in the job queue of the mapped PE.
             * @return ix value, SIZE_MAX else.
             */
            inline u32 jobExecIx() const noexcept { return jobExecIx_; }

            /**
             * @brief Get notification flag for given LRT.
             * @remark no boundary check is performed.
             * @return boolean flag indicating if this task is notifying given LRT.
             */
            inline bool getNotificationFlagForLRT(size_t ix) const { return notifications_.get()[ix]; }

            /**
             * @brief Get the previous task of a given index.
             * @param ix Index of the task.
             * @return pointer to the previous task, nullptr else.
             * @throws @refitem spider::Exception if index out of bound (only in debug)
             */
            Task *previousTask(size_t ix) const;

            /* === Setter(s) === */

            /**
             * @brief Set the start time of the job.
             * @remark This method will overwrite current value.
             * @param time  Value to set.
             */
            void setStartTime(u64 time);

            /**
             * @brief Set the end time of the job.
             * @remark This method will overwrite current value.
             * @param time  Value to set.
             */
            void setEndTime(u64 time);

            /**
            * @brief Set the processing element of the job.
            * @remark This method will overwrite current values.
            * @param mappedPE  Lrt ix inside spider.
            */
            void setMappedPE(const PE *pe);

            /**
             * @brief Set the state of the job.
             * @remark This method will overwrite current value.
             * @param state State to set.
             */
            inline void setState(TaskState state) noexcept { state_ = state; }

            /**
             * @brief Set the execution job index value of the task (that will be used for synchronization).
             * @remark This method will overwrite current values.
             * @param ix Ix to set.
             */
            inline void setJobExecIx(u32 ix) noexcept { jobExecIx_ = ix; }

            /**
             * @brief Set the notification flag for this lrt.
             * @warning There is no check on the value of lrt.
             * @param lrt   Index of the lrt.
             * @param value Value to set: true = should notify, false = should not notify.
             */
            inline void setNotificationFlag(size_t lrt, bool value) {
                notifications_.get()[lrt] = value;
            }

            /**
             * @brief Override the current execution dependency at given position.
             * @param ix   position of the dependency to set.
             * @param task pointer to the task to set.
             * @throws @refitem spider::Exception if index out of bound (only in debug)
             */
            void setExecutionDependency(size_t ix, Task *task);

            /* === Virtual method(s) === */

            /**
             * @brief Get output fifo of index ix.
             * @param ix Index of the Fifo.
             * @return fifo at position ix.
             */
            virtual Fifo getOutputFifo(size_t ix) const;

            /**
             * @brief Get input fifo of index ix.
             * @param ix Index of the Fifo.
             * @return fifo at position ix.
             */
            virtual Fifo getInputFifo(size_t ix) const;

            /**
             * @brief Allocate task memory.
             */
            virtual void allocate(FifoAllocator *) = 0;

            /**
             * @brief Set the ix of the job.
             * @remark This method will overwrite current value.
             * @param ix Ix to set.
             */
            virtual inline void setIx(u32 ix) noexcept { ix_ = ix; }

            /**
             * @brief Flag indicating whether or not a task can be optimized away by a smart fifo allocator.
             * @return true if the task could be optimized away by allocator, false else.
             */
            virtual bool isSyncOptimizable() const noexcept = 0;

            /**
             * @brief Return a color value for the task.
             *        format is RGB with 8 bits per component in the lower part of the returned value.
             * @return  color of the task.
             */
            virtual u32 color() const = 0;

            /**
             * @brief Update task execution dependencies based on schedule information.
             * @param schedule pointer to the schedule.
             */
            virtual void updateTaskExecutionDependencies(const Schedule *schedule) = 0;

            /**
             * @brief Returns the name of the task
             * @return name of the task
             */
            virtual std::string name() const = 0;

            /**
             * @brief Set dependencies notification flag (i.e search which dependencies should send us a notif)
             * @return an array of size of archi::Platform::LRTCount filled with indices to the dependencies that should
             * be notifying us.
             */
            spider::array<size_t> updateDependenciesNotificationFlag() const;

            /**
             * @brief Creates a job message out of the information of the task.
             * @return  JobMessage.
             */
            virtual JobMessage createJobMessage() const;

            /**
             * @brief Compute the communication cost and the data size that would need to be send if a task is mapped
             *        on a given PE.
             * @param mappedPE  PE on which the task is currently mapped.
             * @return pair containing the communication cost as first and the total size of data to send as second.
             */
            virtual std::pair<ufast64, ufast64> computeCommunicationCost(const PE *mappedPE) const = 0;

            /**
             * @brief Check if the task is mappable on a given PE.
             * @param pe  Pointer to the PE.
             * @return true if mappable on PE, false else.
             */
            virtual inline bool isMappableOnPE(const PE */* pe */) const { return true; }

            /**
             * @brief Get the execution timing on a given PE.
             * @param pe  Pointer to the PE.
             * @return exec timing on the PE, UINT64_MAX else.
             */
            virtual inline u64 timingOnPE(const PE */* pe */) const { return UINT64_MAX; }

            /**
             * @brief
             * @return
             */
            virtual DependencyInfo getDependencyInfo(size_t /* ix */) const = 0;

            virtual size_t dependencyCount() const = 0;

        protected:
            spider::unique_ptr<Task *> dependencies_;       /*!< Dependencies of the task */
            spider::unique_ptr<bool> notifications_;        /*!< Notification flags of the task */
            std::shared_ptr<JobFifos> fifos_;               /*!< Fifo(s) attached to the task */
            const PE *mappedPE_{ nullptr };                 /*!< Mapping PE of the task */
            u64 startTime_{ UINT64_MAX };                   /*!< Mapping start time of the task */
            u64 endTime_{ UINT64_MAX };                     /*!< Mapping end time of the task */
            u32 ix_{ UINT32_MAX };                          /*!< Index of the task in the schedule */
            u32 jobExecIx_{ UINT32_MAX };                   /*!< Index of the job sent to the PE */
            TaskState state_{ TaskState::NOT_SCHEDULABLE }; /*!< State of the task */
        };
    }
}

#endif //SPIDER2_TASK_H
