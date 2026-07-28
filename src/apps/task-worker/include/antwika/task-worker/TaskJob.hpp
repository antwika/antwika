#pragma once

#include <cstdint>
#include <string>

#include <antwika/scheduler/IJob.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/task-worker/WorkerLookup.hpp"

namespace antwika::task_worker
{

    using antwika::scheduler::IJob;

    /**
     * @brief A transient scheduling unit: one submitted task.
     *
     * Implements antwika::scheduler::IJob. Carries no dependency state
     * of its own -- dependencies are the Scheduler's concern, resolved
     * before execute() is ever called.
     */
    class TaskJob final : public IJob
    {
    public:
        /**
         * @brief Construct the job over its collaborators and metadata.
         * @param lookup Worker lookup a claimed worker is taken from.
         * Must outlive this job.
         * @param taskId The submission script's own id for this task.
         * @param label A human-readable name for this task.
         * @param durationTicks How many ticks the claimed worker stays
         * busy for.
         */
        TaskJob(
            WorkerLookup &lookup,
            std::uint64_t taskId,
            std::string label,
            antwika::time::Tick durationTicks);

        /**
         * @brief Claim the lowest-index idle worker for this task.
         * @param tick Unused -- claiming doesn't depend on which tick
         * it happens on, only on which worker is currently idle.
         */
        void execute(antwika::time::Tick tick) override;

        /**
         * @brief Get this task's submission-script id.
         * @return The taskId passed at construction.
         */
        [[nodiscard]] std::uint64_t taskId() const noexcept;

        /**
         * @brief Get this task's human-readable label.
         * @return The label passed at construction.
         */
        [[nodiscard]] const std::string &label() const noexcept;

    private:
        WorkerLookup &lookup;
        std::uint64_t id;
        std::string taskLabel;
        antwika::time::Tick durationTicks;
    };

} // namespace antwika::task_worker
