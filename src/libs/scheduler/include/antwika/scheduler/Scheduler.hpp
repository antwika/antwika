#pragma once

#include <cstddef>
#include <memory>
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
        Scheduler() = default;

        // Owns the jobs given to the unique_ptr schedule() overload.
        // Copying would double-free them.
        // Moving would relocate what live IJob& borrowers point at.
        Scheduler(const Scheduler &) = delete;
        Scheduler(Scheduler &&) = delete;

        Scheduler &operator=(const Scheduler &) = delete;
        Scheduler &operator=(Scheduler &&) = delete;

        /**
         * @brief Enqueue a job this Scheduler takes ownership of.
         * @param job The job to run later; the Scheduler keeps it alive
         * until the Scheduler itself is destroyed. Prefer this overload
         * over the IJob& one whenever the job is heap-allocated purely
         * to be scheduled, since it makes the lifetime rule impossible
         * to get wrong.
         * @param priority The job's priority; higher runs first.
         * @param dependsOn JobIds (issued by this same Scheduler) that
         * must run before this job becomes a candidate for run().
         * Duplicate entries are harmless. An entry this Scheduler
         * never issued throws SchedulerError, and nothing is mutated
         * -- job is released rather than retained.
         * @return The newly assigned JobId, strictly greater than
         * every JobId issued before it.
         * @throws SchedulerError if job is null, or if dependsOn names
         * a JobId this Scheduler never issued.
         */
        JobId schedule(
            std::unique_ptr<IJob> job,
            Priority priority,
            std::vector<JobId> dependsOn = {});

        /**
         * @brief Enqueue a job owned by the caller.
         * @param job The job to run later; the Scheduler stores a
         * non-owning pointer, so the caller must keep it alive until
         * this Scheduler is destroyed -- outliving the job's own run()
         * is not enough, since records keeps the pointer either way.
         * Only correct when the job provably outlives this Scheduler
         * (a longer-lived member, or a stack object declared before
         * it); otherwise use the unique_ptr overload above.
         * @param priority The job's priority; higher runs first.
         * @param dependsOn JobIds (issued by this same Scheduler) that
         * must run before this job becomes a candidate for run().
         * Duplicate entries are harmless. An entry this Scheduler
         * never issued throws SchedulerError, and nothing is mutated.
         * @return The newly assigned JobId, strictly greater than
         * every JobId issued before it.
         * @throws SchedulerError if dependsOn names a JobId this
         * Scheduler never issued.
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

        // Jobs handed over by the owning schedule() overload.
        // Declared first so it is destroyed last.
        // records below holds raw IJob* into these.
        std::vector<std::unique_ptr<IJob>> ownedJobs;
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
