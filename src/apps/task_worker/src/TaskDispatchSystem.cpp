#include "antwika/task_worker/TaskDispatchSystem.hpp"

namespace antwika::task_worker
{

    TaskDispatchSystem::TaskDispatchSystem(
        Scheduler &jobScheduler, WorkerLookup &lookup)
        : jobScheduler(jobScheduler), lookup(lookup)
    {
    }

    void TaskDispatchSystem::update(World &, antwika::time::Tick tick)
    {
        lookup.refresh();
        jobScheduler.run(tick, lookup.idleCount());
    }

} // namespace antwika::task_worker
