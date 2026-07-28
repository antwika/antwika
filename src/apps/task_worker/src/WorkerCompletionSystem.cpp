#include "antwika/task_worker/WorkerCompletionSystem.hpp"

#include "antwika/task_worker/Worker.hpp"

namespace antwika::task_worker
{

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
                world.set<Worker>(entity, Worker{WorkerStatus::Idle, 0});
            }
            else
            {
                world.set<Worker>(
                    entity,
                    Worker{
                        WorkerStatus::Busy, worker.remainingTicks - 1});
            }
        }
    }

} // namespace antwika::task_worker
