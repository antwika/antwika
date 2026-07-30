#pragma once

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/task_worker/TaskRegistry.hpp"

namespace antwika::task_worker
{

    using antwika::ecs::ISystem;
    using antwika::ecs::World;

    /**
     * @brief Frees workers whose busy countdown has run out.
     *
     * Runs in the "release" phase, before "dispatch", so a worker that
     * finishes this tick is immediately available to pick up new work
     * this same tick, not one tick late.
     */
    class WorkerCompletionSystem final : public ISystem
    {
    public:
        /**
         * @brief Construct the system over the registry it reports
         * task progress to.
         * @param registry Task registry updated with every Busy
         * worker's new countdown, or marked completed at zero.
         */
        explicit WorkerCompletionSystem(TaskRegistry &registry);

        WorkerCompletionSystem(const WorkerCompletionSystem &) = delete;
        WorkerCompletionSystem(WorkerCompletionSystem &&) = delete;

        WorkerCompletionSystem &operator=(
            const WorkerCompletionSystem &) = delete;
        WorkerCompletionSystem &operator=(WorkerCompletionSystem &&) = delete;

        /**
         * @brief Decrement every Busy worker's countdown by one, flip
         * to Idle at zero, keeping registry's view of each task's
         * progress in sync either way.
         * @param world World read from and staged into.
         * @param tick Unused -- release only depends on each worker's
         * own countdown, not on which tick it's being applied for.
         */
        void update(World &world, antwika::time::Tick tick) override;

    private:
        TaskRegistry &registry;
    };

} // namespace antwika::task_worker
