#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <antwika/scheduler/JobId.hpp>
#include <antwika/scheduler/Priority.hpp>
#include <antwika/time/Tick.hpp>

namespace antwika::task_worker
{

    /**
     * @brief A task's lifecycle stage, from the job scheduler's point
     * of view: waiting for a worker, currently occupying one, or done.
     */
    enum class TaskStatus : std::uint8_t
    {
        Pending,
        Running,
        Completed,
    };

    /**
     * @brief A task's full status for human-readable reporting: its
     * identity, priority, lifecycle stage, and however many ticks are
     * left (its full requested duration while Pending, the live
     * countdown while Running, zero once Completed).
     */
    struct TaskInfo
    {
        std::uint64_t taskId;
        std::string label;
        antwika::scheduler::Priority priority;
        TaskStatus status;
        antwika::time::Tick remainingTicks;

        bool operator==(const TaskInfo &other) const = default;
    };

    /**
     * @brief Tracks every submitted task's full status across its
     * lifecycle, for human-readable output.
     *
     * Purely an app-layer reporting aid, not World state: unlike
     * Worker, nothing here needs to be an antwika::ecs::Component, so
     * it's free to hold std::string/std::vector directly.
     */
    class TaskRegistry final
    {
    public:
        /**
         * @brief Record a newly scheduled task, Pending, with its
         * requested duration standing in for "ticks left" until it
         * starts.
         * @param taskId The submission script's own id for the task.
         * @param label The task's human-readable label.
         * @param priority The priority it was scheduled at.
         * @param durationTicks How many ticks it will occupy a worker
         * for, once started.
         *
         * Callers must submit tasks in the same order the owning
         * antwika::scheduler::Scheduler hands out JobIds (i.e. once per
         * Scheduler::schedule() call, immediately after it): the Nth
         * submit() call is assumed to describe the task with JobId N,
         * matching how TaskSubmissionSink's own `jobs` vector already
         * lines up with JobId by construction.
         */
        void submit(
            std::uint64_t taskId,
            std::string label,
            antwika::scheduler::Priority priority,
            antwika::time::Tick durationTicks);

        /**
         * @brief Mark a task Running (dispatched to a worker).
         * @param jobId The JobId antwika::scheduler::Scheduler::run()
         * reported as executed. An id with no corresponding submit()
         * call (e.g. a job scheduled without going through this
         * registry) is silently ignored rather than reported.
         */
        void markStarted(antwika::scheduler::JobId jobId);

        /**
         * @brief Update a Running task's live countdown.
         * @param taskId The task's submission-script id.
         * @param remainingTicks Ticks left on its worker's countdown.
         * A taskId with no corresponding submit() call is silently
         * ignored rather than reported.
         */
        void updateRemaining(
            std::uint64_t taskId, antwika::time::Tick remainingTicks);

        /**
         * @brief Mark a task Completed, zeroing its ticks left.
         * @param taskId The finished task's submission-script id. A
         * taskId with no corresponding submit() call is silently
         * ignored rather than reported.
         */
        void markCompleted(std::uint64_t taskId);

        /**
         * @brief Get every submitted task's current status.
         * @return Tasks in submission (ascending JobId) order.
         */
        [[nodiscard]] const std::vector<TaskInfo> &allTasks() const noexcept;

    private:
        [[nodiscard]] TaskInfo *findByTaskId(std::uint64_t taskId);

        // Indexed by rawValue(jobId) - 1.
        std::vector<TaskInfo> entries;
    };

} // namespace antwika::task_worker
