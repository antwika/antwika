#include "antwika/task_worker/TaskRegistry.hpp"

#include <algorithm>

namespace antwika::task_worker
{

    void TaskRegistry::submit(
        std::uint64_t taskId,
        std::string label,
        antwika::scheduler::Priority priority,
        antwika::time::Tick durationTicks,
        std::optional<TaskDependency> dependsOn)
    {
        entries.push_back(TaskInfo{ // GCOVR_EXCL_LINE
            taskId, std::move(label), priority, TaskStatus::Pending,
            durationTicks, durationTicks, std::move(dependsOn)});
        startTargets.push_back(entries.size() - 1);
    }

    void TaskRegistry::noteDispatch(
        std::size_t budget, std::size_t dispatched)
    {
        dispatch = DispatchInfo{budget, dispatched};
    }

    void TaskRegistry::markStarted(antwika::scheduler::JobId jobId)
    {
        const auto index = antwika::scheduler::rawValue(jobId) - 1;
        if (index < startTargets.size())
        {
            entries[startTargets[index]].status = TaskStatus::Running;
        }
    }

    void TaskRegistry::restore(
        std::vector<TaskInfo> tasks, DispatchInfo lastDispatch)
    {
        entries = std::move(tasks);
        dispatch = lastDispatch;

        startTargets.clear();
        for (std::size_t index = 0; index < entries.size(); ++index)
        {
            if (entries[index].status == TaskStatus::Pending)
            {
                startTargets.push_back(index);
            }
        }
    }

    void TaskRegistry::updateRemaining(
        std::uint64_t taskId, antwika::time::Tick remainingTicks)
    {
        auto *entry = findByTaskId(taskId);
        if (entry != nullptr)
        {
            entry->remainingTicks = remainingTicks;
        }
    }

    void TaskRegistry::markCompleted(std::uint64_t taskId)
    {
        auto *entry = findByTaskId(taskId);
        if (entry != nullptr)
        {
            entry->status = TaskStatus::Completed;
            entry->remainingTicks = 0;
        }
    }

    const std::vector<TaskInfo> &TaskRegistry::allTasks() const noexcept
    {
        return entries;
    }

    DispatchInfo TaskRegistry::lastDispatch() const noexcept
    {
        return dispatch;
    }

    TaskInfo *TaskRegistry::findByTaskId(std::uint64_t taskId)
    {
        const auto it = std::find_if(
            entries.begin(),
            entries.end(),
            [taskId](const TaskInfo &entry)
            { return entry.taskId == taskId; });
        return it != entries.end() ? &(*it) : nullptr;
    }

}
