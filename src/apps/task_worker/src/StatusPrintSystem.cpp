#include "antwika/task_worker/StatusPrintSystem.hpp"

#include <antwika/ecs_commons/Name.hpp>
#include <antwika/scheduler/Priority.hpp>

#include "antwika/task_worker/Worker.hpp"

namespace antwika::task_worker
{

    namespace
    {

        const char *statusName(TaskStatus status)
        {
            switch (status) // GCOVR_EXCL_LINE
            {
            case TaskStatus::Pending:
                return "Pending";
            case TaskStatus::Running:
                return "Running";
            case TaskStatus::Completed:
                return "Completed";
            }
            return "Unknown"; // GCOVR_EXCL_LINE
        }

    }

    StatusPrintSystem::StatusPrintSystem(
        std::ostream &out, TaskRegistry &registry)
        : out(out), registry(registry)
    {
    }

    void StatusPrintSystem::update(World &world, antwika::time::Tick tick)
    {
        out << "After tick " << tick << ":\n";

        out << "  Tasks:\n";
        for (const auto &task : registry.allTasks())
        {
            out << "    Task id: " << task.taskId
                << " | Task name: " << task.label << " | Priority: "
                << static_cast<unsigned>(
                       antwika::scheduler::rawValue(task.priority))
                << " | Status: " << statusName(task.status)
                << " | Remaining: " << task.remainingTicks << " tick(s)";
            if (task.dependsOn.has_value())
            {
                out << " | Depends on: " << task.dependsOn->label << " ("
                    << task.dependsOn->taskId << ")";
            }
            out << "\n";
        }

        out << "  Workers:\n";
        std::size_t index = 0;
        for (const auto entity : allWorkers(world))
        {
            const auto &worker = world.get<Worker>(entity);
            out << "    worker[" << index << "] - Current state: "
                << (worker.status == WorkerStatus::Idle ? "Idle" : "Busy");
            if (worker.status == WorkerStatus::Busy)
            {
                out << " | Remaining: " << worker.remainingTicks
                    << " tick(s) | Task id: " << worker.taskId
                    << " | Task name: "
                    << antwika::ecs_commons::view(worker.label);
            }
            out << "\n";
            ++index;
        }
    }

}
