#include "antwika/task-worker/WorkerLookup.hpp"

#include <algorithm>

#include "antwika/task-worker/Worker.hpp"

namespace antwika::task_worker
{

    WorkerLookup::WorkerLookup(World &world, std::vector<Entity> workers)
        : world(world),
          workers(std::move(workers)),
          idle(this->workers.size(), true)
    {
    }

    void WorkerLookup::refresh()
    {
        for (std::size_t i = 0; i < workers.size(); ++i)
        {
            idle[i] = world.get<Worker>(workers[i]).status ==
                      WorkerStatus::Idle;
        }
    }

    std::size_t WorkerLookup::idleCount() const noexcept
    {
        return static_cast<std::size_t>(
            std::count(idle.begin(), idle.end(), true));
    }

    bool WorkerLookup::claimIdle(antwika::time::Tick durationTicks)
    {
        const auto it = std::find(idle.begin(), idle.end(), true);
        if (it == idle.end())
        {
            return false; // GCOVR_EXCL_LINE
        }

        const auto index =
            static_cast<std::size_t>(std::distance(idle.begin(), it));
        idle[index] = false;
        world.set<Worker>(
            workers[index], Worker{WorkerStatus::Busy, durationTicks});
        return true;
    }

} // namespace antwika::task_worker
