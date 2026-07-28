#pragma once

#include <cstddef>
#include <vector>

#include <antwika/time/Tick.hpp>

#include "antwika/scheduler/IJob.hpp"
#include "antwika/scheduler/JobId.hpp"
#include "antwika/scheduler/Priority.hpp"

namespace antwika::scheduler
{

    /**
     * @brief Runs enqueued jobs in deterministic priority order,
     * distributed across ticks by a per-call budget, honoring a
     * dependency DAG between jobs.
     *
     * Single-threaded, no RNG, no hashing anywhere in the pending-job
     * or dependency-tracking path.
     */
    class Scheduler final
    {
    public:
        /**
         * @brief Enqueue a job.
         * @param job The job to run later; the Scheduler stores a
         * non-owning pointer, so the caller must keep it alive until
         * it's either run or this Scheduler is destroyed.
         * @param priority The job's priority; higher runs first.
         * @param dependsOn JobIds (issued by this same Scheduler) that
         * must run before this job becomes a candidate for run().
         * Duplicate entries are harmless. An entry this Scheduler
         * never issued throws SchedulerError, and nothing is mutated.
         * @return The newly assigned JobId, strictly greater than
         * every JobId issued before it.
         */
        JobId schedule(
            IJob &job,
            Priority priority,
            std::vector<JobId> dependsOn = {});

        /**
         * @brief Execute up to budget ready jobs, highest priority
         * first, equal priority FIFO by submission order.
         * @param tick Forwarded to each executed job's execute().
         * @param budget The maximum number of jobs to execute this
         * call; 0 is a valid no-op.
         * @return The JobIds executed, in the order they ran. Only
         * jobs known to this Scheduler before this call began are
         * ever eligible, no matter what triggers their readiness
         * during the call.
         */
        std::vector<JobId> run(antwika::time::Tick tick, std::size_t budget);

        /**
         * @brief Count jobs scheduled but not yet run.
         * @return The number of jobs still pending, ready or blocked.
         */
        [[nodiscard]] std::size_t pending() const noexcept;

        /**
         * @brief Check whether there is nothing left to run.
         * @return True iff pending() == 0.
         */
        [[nodiscard]] bool empty() const noexcept;

    private:
        struct Entry
        {
            JobId id;
            Priority priority;
        };

        struct JobRecord
        {
            Priority priority;
            IJob *job;
            bool completed;
        };

        void insertReady(JobId id, Priority priority);

        // Ready to run, kept sorted by (priority desc, id asc).
        std::vector<Entry> ready;
        // Indexed by rawValue(id) - 1.
        std::vector<JobRecord> records;
        // Indexed by rawValue(id) - 1: remaining unmet dependencies.
        std::vector<std::size_t> unmetCount;
        // Indexed by rawValue(id) - 1: jobs waiting on this one.
        std::vector<std::vector<JobId>> dependents;

        std::size_t pendingCount{0};
        JobId nextId{1};
    };

} // namespace antwika::scheduler
