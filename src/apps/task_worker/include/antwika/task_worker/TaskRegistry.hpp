#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
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
     * @brief The identity of a task another task depends on, denormalized
     * onto the dependent so reporting never needs a second lookup.
     */
    struct TaskDependency
    {
        std::uint64_t taskId;
        std::string label;

        bool operator==(const TaskDependency &other) const = default;
    };

    /**
     * @brief A task's full status for human-readable reporting: its
     * identity, priority, lifecycle stage, the duration it was submitted
     * with, however many ticks are left (its full requested duration
     * while Pending, the live countdown while Running, zero once
     * Completed), and the task it depends on, if any.
     */
    struct TaskInfo
    {
        std::uint64_t taskId;
        std::string label;
        antwika::scheduler::Priority priority;
        TaskStatus status;

        // Kept beside the countdown rather than derived from it.
        // Running, remainingTicks is all that is left of the number.
        // How far through a task is needs both to be worked out.
        antwika::time::Tick durationTicks;
        antwika::time::Tick remainingTicks;
        std::optional<TaskDependency> dependsOn;

        bool operator==(const TaskInfo &other) const = default;
    };

    /**
     * @brief What the last dispatch was allowed to start, and did.
     *
     * The budget is the idle-worker count antwika::scheduler::Scheduler
     * was run with, which is the one number that decides how many jobs
     * move at all -- and the one thing about a tick's dispatch that no
     * task carries afterwards.
     */
    struct DispatchInfo
    {
        std::size_t budget{0};
        std::size_t dispatched{0};

        bool operator==(const DispatchInfo &other) const = default;
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
         * @param dependsOn The task it must wait on, if any.
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
            antwika::time::Tick durationTicks,
            std::optional<TaskDependency> dependsOn = std::nullopt);

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
         * @brief Record what one tick's dispatch was allowed to start.
         * @param budget The idle-worker count the job scheduler was run
         * with.
         * @param dispatched How many jobs that call actually started.
         *
         * Observed here rather than worked out from the task list,
         * which cannot say how much of a budget went unspent: a tick
         * with two idle workers and one ready job looks exactly like a
         * tick with one of each once it is over.
         */
        void noteDispatch(std::size_t budget, std::size_t dispatched);

        /**
         * @brief Replace every entry with a loaded dump's task list.
         * @param tasks The tasks, in their original submission order.
         * @param lastDispatch The dispatch the dump was taken after.
         *
         * The caller re-schedules every Pending entry, in this list's
         * order, on a fresh scheduler whose JobIds start at 1 again --
         * so the Nth Pending entry holds new JobId N, and this call
         * records exactly that numbering.
         * That shared invariant is what keeps markStarted() resolving
         * a renumbered JobId to the right task afterwards; see
         * TaskWorkerSnapshotStore, the one caller.
         */
        void restore(
            std::vector<TaskInfo> tasks, DispatchInfo lastDispatch);

        /**
         * @brief Get every submitted task's current status.
         * @return Tasks in submission (ascending JobId) order.
         */
        [[nodiscard]] const std::vector<TaskInfo> &allTasks() const noexcept;

        /**
         * @brief Get the last dispatch this registry was told about.
         * @return That dispatch, or a zero budget before the first one.
         */
        [[nodiscard]] DispatchInfo lastDispatch() const noexcept;

    private:
        [[nodiscard]] TaskInfo *findByTaskId(std::uint64_t taskId);

        // In submission order, which restore() preserves.
        std::vector<TaskInfo> entries;

        // Maps rawValue(jobId) - 1 to an index into entries.
        // Identity until a restore renumbers the pending tasks.
        std::vector<std::size_t> startTargets;

        DispatchInfo dispatch;
    };

} // namespace antwika::task_worker
