#pragma once

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/time/Tick.hpp>

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
         * @brief Decrement every Busy worker's countdown by one, flip
         * to Idle at zero.
         * @param world World read from and staged into.
         * @param tick Unused -- release only depends on each worker's
         * own countdown, not on which tick it's being applied for.
         */
        void update(World &world, antwika::time::Tick tick) override;
    };

} // namespace antwika::task_worker
