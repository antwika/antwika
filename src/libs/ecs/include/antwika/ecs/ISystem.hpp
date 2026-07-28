#pragma once

#include <antwika/time/Tick.hpp>

#include "antwika/ecs/World.hpp"

namespace antwika::ecs
{

    using antwika::time::Tick;

    /**
     * @brief One unit of per-tick behavior over a World.
     *
     * update() only ever gets a World&, whose API makes "read front,
     * write back" the only thing reachable — there's no way for a
     * system to affect what any other system observes during the same
     * phase (see World, ComponentStorage).
     */
    class ISystem
    {
    public:
        virtual ~ISystem() = default;

        /**
         * @brief Run this system's behavior for one tick.
         * @param world The world to read from and stage writes into.
         * @param tick The tick being processed.
         */
        virtual void update(World &world, Tick tick) = 0;
    };

} // namespace antwika::ecs
