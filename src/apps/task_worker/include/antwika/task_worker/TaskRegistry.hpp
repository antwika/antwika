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

    enum class TaskStatus : std::uint8_t
    {
        Pending,
        Running,
        Completed,
    };

    [[nodiscard]] constexpr TaskStatus enumBound(TaskStatus) noexcept
    {
        return TaskStatus::Completed;
    }

    struct TaskDependency final
    {
        std::uint64_t taskId;
        std::string label;

        bool operator==(const TaskDependency &other) const = default;
    };

    struct TaskInfo final
    {
        std::uint64_t taskId;
        std::string label;
        antwika::scheduler::Priority priority;
        TaskStatus status;

        antwika::time::Tick durationTicks;
        antwika::time::Tick remainingTicks;
        std::optional<TaskDependency> dependsOn;

        bool operator==(const TaskInfo &other) const = default;
    };

    struct DispatchInfo final
    {
        std::size_t budget{0};
        std::size_t dispatched{0};

        bool operator==(const DispatchInfo &other) const = default;
    };

    class TaskRegistry final
    {
    public:
        void submit(
            std::uint64_t taskId,
            std::string label,
            antwika::scheduler::Priority priority,
            antwika::time::Tick durationTicks,
            std::optional<TaskDependency> dependsOn = std::nullopt);

        void markStarted(antwika::scheduler::JobId jobId);

        void updateRemaining(
            std::uint64_t taskId, antwika::time::Tick remainingTicks);

        void markCompleted(std::uint64_t taskId);

        void noteDispatch(std::size_t budget, std::size_t dispatched);

        void restore(
            std::vector<TaskInfo> tasks, DispatchInfo lastDispatch);

        [[nodiscard]] const std::vector<TaskInfo> &allTasks() const noexcept;

        [[nodiscard]] DispatchInfo lastDispatch() const noexcept;

    private:
        [[nodiscard]] TaskInfo *findByTaskId(std::uint64_t taskId);

        std::vector<TaskInfo> entries;

        std::vector<std::size_t> startTargets;

        DispatchInfo dispatch;
    };

}
