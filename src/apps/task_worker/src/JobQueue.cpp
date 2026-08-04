#include "antwika/task_worker/JobQueue.hpp"

namespace antwika::task_worker
{

    JobQueue::JobQueue() : current(std::make_unique<Scheduler>())
    {
    }

    Scheduler &JobQueue::scheduler() noexcept
    {
        return *current;
    }

    Scheduler &JobQueue::reset()
    {
        current = std::make_unique<Scheduler>();
        return *current;
    }

} // namespace antwika::task_worker
