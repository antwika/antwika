#include "antwika/task-worker/WorkerStatusPrintSystem.hpp"

#include "antwika/task-worker/Worker.hpp"

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
            out << "  worker[" << index << "]: "
                << (worker.status == WorkerStatus::Idle ? "Idle"
                                                          : "Busy")
                << " remaining=" << worker.remainingTicks << "\n";
            ++index;
        }
    }

} // namespace antwika::task_worker
