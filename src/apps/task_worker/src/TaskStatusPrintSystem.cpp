#include "antwika/task_worker/TaskStatusPrintSystem.hpp"

#include <antwika/scheduler/Priority.hpp>

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

    } // namespace

    TaskStatusPrintSystem::TaskStatusPrintSystem(
        std::ostream &out, TaskRegistry &registry)
        : out(out), registry(registry)
    {
    }

    void TaskStatusPrintSystem::update(World &, antwika::time::Tick tick)
    {
        out << "Tasks after tick " << tick << ":\n";

        for (const auto &task : registry.allTasks())
        {
            out << "  Task id: " << task.taskId
                << " | Task name: " << task.label << " | Priority: "
                << static_cast<unsigned>(
                       antwika::scheduler::rawValue(task.priority))
                << " | Status: " << statusName(task.status)
                << " | Remaining: " << task.remainingTicks << " tick(s)\n";
        }
    }

} // namespace antwika::task_worker
