#include "antwika/task_worker/WorkerStatusPrintSystem.hpp"

#include "antwika/task_worker/Worker.hpp"

namespace antwika::task_worker
{

    WorkerStatusPrintSystem::WorkerStatusPrintSystem(std::ostream &out)
        : out(out)
    {
    }

    void WorkerStatusPrintSystem::update(
        World &world, antwika::time::Tick tick)
    {
        out << "After tick " << tick << ":\n";

        std::size_t index = 0;
        for (const auto entity : world.view<Worker>())
        {
            const auto &worker = world.get<Worker>(entity);
            out << "  worker[" << index << "] - Current state: "
                << (worker.status == WorkerStatus::Idle ? "Idle" : "Busy");
            if (worker.status == WorkerStatus::Busy)
            {
                out << " | Remaining: " << worker.remainingTicks
                    << " tick(s) | Task id: " << worker.taskId
                    << " | Task name: " << worker.label.data();
            }
            out << "\n";
            ++index;
        }
    }

} // namespace antwika::task_worker
