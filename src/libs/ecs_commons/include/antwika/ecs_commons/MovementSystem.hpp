#pragma once

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/time/Tick.hpp>

namespace antwika::ecs_commons
{

    using antwika::ecs::ISystem;
    using antwika::ecs::World;

    /**
     * @brief Moves every entity with a GridPosition and a Velocity one
     * step along, once per tick.
     *
     * Reads only the front buffer and stages into the back, so no entity
     * ever moves in response to another entity's move within the same
     * tick: two entities crossing pass through each other, and the result
     * does not depend on which was created first.
     * Iteration order is View's, which is ComponentStorage's insertion
     * order, so the order is the same every run.
     *
     * The system holds no state of its own, which is why it takes no
     * constructor arguments: everything it needs is in the World it is
     * handed.
     */
    class MovementSystem final : public ISystem
    {
    public:
        /**
         * @brief Advance every moving entity by its velocity.
         * @param world The world to read positions from and stage new
         * ones into.
         * @param tick The tick being processed; unused, since a step
         * depends only on the entity's own velocity.
         */
        void update(World &world, antwika::time::Tick tick) override;
    };

} // namespace antwika::ecs_commons
