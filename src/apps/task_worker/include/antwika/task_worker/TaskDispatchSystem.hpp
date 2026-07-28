#pragma once

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/scheduler/Scheduler.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/task_worker/WorkerLookup.hpp"

namespace antwika::task_worker
{

    using antwika::ecs::ISystem;
    using antwika::ecs::World;
    using antwika::scheduler::Scheduler;

    /**
     * @brief Dispatches ready jobs to idle workers, once per tick.
     *
     * Runs in the "dispatch" phase, after "release". Lives at the
     * application layer, not in antwika::scheduler, per the library's
     * decision to stay independent of antwika::ecs (see
     * PLAN_SCHEDULER.md §3.8): this is the ECS integration.
     */
    class TaskDispatchSystem final : public ISystem
    {
    public:
        /**
         * @brief Construct the system over its collaborators.
         * @param jobScheduler Scheduler run() is called against.
         * @param lookup Worker lookup refreshed and used to compute
         * this tick's dispatch budget.
         */
        TaskDispatchSystem(Scheduler &jobScheduler, WorkerLookup &lookup);

        /**
         * @brief Refresh idle-worker state, then run the job scheduler
         * with the current idle-worker count as budget.
         * @param world Unused directly -- idle state comes from lookup,
         * which was built over the same World.
         * @param tick Forwarded to Scheduler::run().
         */
        void update(World &world, antwika::time::Tick tick) override;

    private:
        Scheduler &jobScheduler;
        WorkerLookup &lookup;
    };

} // namespace antwika::task_worker
