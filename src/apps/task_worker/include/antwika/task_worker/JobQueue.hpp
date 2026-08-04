#pragma once

#include <memory>

#include <antwika/scheduler/Scheduler.hpp>

namespace antwika::task_worker
{

    using antwika::scheduler::Scheduler;

    /**
     * @brief Owns the run's one job scheduler, replaceably.
     *
     * antwika::scheduler::Scheduler cannot be cleared, copied or
     * moved, and its ids only ever count up -- which is exactly right
     * for a run, and exactly wrong for load_state, which must discard
     * whatever the current run had pending.
     * This holder is the seam that squares the two: everything that
     * schedules or runs jobs reaches the scheduler through it, so a
     * restore can swap in a fresh one, ids starting over at 1, without
     * reseating anybody's reference.
     */
    class JobQueue final
    {
    public:
        /**
         * @brief Construct the queue over a fresh scheduler.
         */
        JobQueue();

        JobQueue(const JobQueue &) = delete;
        JobQueue(JobQueue &&) = delete;

        JobQueue &operator=(const JobQueue &) = delete;
        JobQueue &operator=(JobQueue &&) = delete;

        /**
         * @brief Get the scheduler currently in service.
         * @return The scheduler; valid until the next reset().
         */
        [[nodiscard]] Scheduler &scheduler() noexcept;

        /**
         * @brief Replace the scheduler with a fresh, empty one.
         *
         * Every pending job is dropped with it, and the fresh one
         * hands out JobIds from 1 again -- the renumbering the whole
         * restore is built around, see TaskWorkerSnapshotStore.
         *
         * @return The fresh scheduler, for the caller about to fill it.
         */
        Scheduler &reset();

    private:
        std::unique_ptr<Scheduler> current;
    };

} // namespace antwika::task_worker
