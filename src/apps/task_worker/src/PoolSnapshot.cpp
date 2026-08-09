#include "antwika/task_worker/PoolSnapshot.hpp"

#include <algorithm>
#include <string>

#include <antwika/ecs_commons/Name.hpp>
#include <antwika/scheduler/Priority.hpp>

namespace antwika::task_worker
{

    namespace
    {

        [[nodiscard]] const TaskInfo *findTask(
            const std::vector<TaskInfo> &tasks, std::uint64_t taskId)
        {
            const auto it = std::find_if(
                tasks.begin(),
                tasks.end(),
                [taskId](const TaskInfo &task)
                { return task.taskId == taskId; });
            return it != tasks.end() ? &(*it) : nullptr;
        }

        [[nodiscard]] bool blockedBy(
            const std::vector<TaskInfo> &tasks, const TaskInfo &task)
        {
            if (!task.dependsOn.has_value())
            {
                return false;
            }

            const auto *dependency =
                findTask(tasks, task.dependsOn->taskId);
            return dependency == nullptr
                   || dependency->status != TaskStatus::Completed;
        }

        [[nodiscard]] TaskView viewOf(
            const TaskInfo &task, bool blocked)
        {
            return TaskView{ // GCOVR_EXCL_LINE
                .taskId = task.taskId,
                .label = task.label,
                .priority = task.priority,
                .durationTicks = task.durationTicks,
                .blocked = blocked,
                .waitingFor = blocked ? task.dependsOn->label
                                      : std::string{}};
        }

        [[nodiscard]] WorkerView viewOf(
            const Worker &worker, const std::vector<TaskInfo> &tasks)
        {
            if (worker.status != WorkerStatus::Busy)
            {
                return WorkerView{}; // GCOVR_EXCL_LINE
            }

            const auto *task = findTask(tasks, worker.taskId);

            return WorkerView{ // GCOVR_EXCL_LINE
                .status = WorkerStatus::Busy,
                .taskId = worker.taskId,
                .label =
                    std::string{antwika::ecs_commons::view(worker.label)},
                .durationTicks =
                    task != nullptr ? task->durationTicks : 0,
                .remainingTicks = worker.remainingTicks};
        }

    }

    PoolSnapshot snapshotOf(
        const antwika::ecs::World &world,
        const TaskRegistry &registry,
        antwika::time::Tick tick)
    {
        const auto &tasks = registry.allTasks();

        PoolSnapshot snapshot{
            .tick = tick,
            .dispatch = registry.lastDispatch(),
            .workers = {},
            .queue = {},
            .completed = {}};

        for (const auto entity : allWorkers(world))
        {
            snapshot.workers.push_back(
                viewOf(world.get<Worker>(entity), tasks));
        }

        std::vector<TaskView> blocked;

        for (const auto &task : tasks)
        {
            if (task.status == TaskStatus::Completed)
            {
                snapshot.completed.push_back(viewOf(task, false));
                continue;
            }

            if (task.status != TaskStatus::Pending)
            {
                continue;
            }

            if (blockedBy(tasks, task))
            {
                blocked.push_back(viewOf(task, true));
                continue;
            }

            snapshot.queue.push_back(viewOf(task, false));
        }

        std::stable_sort(
            snapshot.queue.begin(),
            snapshot.queue.end(),
            [](const TaskView &left, const TaskView &right)
            {
                return antwika::scheduler::rawValue(left.priority)
                       > antwika::scheduler::rawValue(right.priority);
            });

        snapshot.queue.insert(
            snapshot.queue.end(), blocked.begin(), blocked.end());

        return snapshot;
    }

}
