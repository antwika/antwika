#include "antwika/task_worker/TaskDispatchSystem.hpp"

namespace antwika::task_worker
{

    TaskDispatchSystem::TaskDispatchSystem(
        JobQueue &jobs,
        WorkerLookup &lookup,
        TaskRegistry &registry)
        : jobs(jobs), lookup(lookup), registry(registry)
    {
    }

    void TaskDispatchSystem::update(World &, antwika::time::Tick tick)
    {
        lookup.refresh();
        const auto budget = lookup.idleCount();
        const auto executed = jobs.scheduler().run(tick, budget);
        for (const auto jobId : executed)
        {
            registry.markStarted(jobId);
        }
        registry.noteDispatch(budget, executed.size());
    }

} // namespace antwika::task_worker
