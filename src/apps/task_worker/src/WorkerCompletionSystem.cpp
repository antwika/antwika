#include "antwika/task_worker/WorkerCompletionSystem.hpp"

#include "antwika/task_worker/Worker.hpp"

namespace antwika::task_worker
{

    WorkerCompletionSystem::WorkerCompletionSystem(TaskRegistry &registry)
        : registry(registry)
    {
    }

    void WorkerCompletionSystem::update(World &world, antwika::time::Tick)
    {
        for (const auto entity : world.view<Worker>())
        {
            const auto worker = world.get<Worker>(entity);
            if (worker.status != WorkerStatus::Busy)
            {
                continue;
            }

            if (worker.remainingTicks <= 1)
            {
                registry.markCompleted(worker.taskId);
                world.set<Worker>(entity, Worker{WorkerStatus::Idle, 0});
            }
            else
            {
                const auto newRemaining = worker.remainingTicks - 1;
                registry.updateRemaining(worker.taskId, newRemaining);
                world.set<Worker>(
                    entity,
                    Worker{
                        WorkerStatus::Busy, newRemaining, worker.taskId,
                        worker.label});
            }
        }
    }

} // namespace antwika::task_worker
