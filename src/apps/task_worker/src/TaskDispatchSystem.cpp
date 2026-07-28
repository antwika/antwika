#include "antwika/task_worker/TaskDispatchSystem.hpp"

namespace antwika::task_worker
{

    TaskDispatchSystem::TaskDispatchSystem(
        Scheduler &jobScheduler,
        WorkerLookup &lookup,
        TaskRegistry &registry)
        : jobScheduler(jobScheduler), lookup(lookup), registry(registry)
    {
    }

    void TaskDispatchSystem::update(World &, antwika::time::Tick tick)
    {
        lookup.refresh();
        const auto executed = jobScheduler.run(tick, lookup.idleCount());
        for (const auto jobId : executed)
        {
            registry.markStarted(jobId);
        }
    }

} // namespace antwika::task_worker
