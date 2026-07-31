#include "antwika/task_worker/TaskJob.hpp"

namespace antwika::task_worker
{

    TaskJob::TaskJob(
        WorkerLookup &lookup,
        std::uint64_t taskId,
        std::string label,
        antwika::time::Tick durationTicks)
        : lookup(lookup),
          id(taskId),
          taskLabel(std::move(label)),
          durationTicks(durationTicks)
    {
    }

    void TaskJob::execute(antwika::time::Tick)
    {
        lookup.claimIdle(durationTicks, id, taskLabel);
    }

} // namespace antwika::task_worker
