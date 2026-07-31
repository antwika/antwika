#pragma once

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/time/Tick.hpp>

namespace antwika::ecs_commons
{

    using antwika::ecs::ISystem;
    using antwika::ecs::World;

    /**
     * @brief Counts every Lifetime down by one tick and destroys the
     * entity when it runs out.
     *
     * A Lifetime of n survives n ticks: the nth update destroys it.
     * Zero and one therefore both expire on the first update, since an
     * entity with nothing left cannot be given a tick back.
     *
     * Destruction is staged, like every other write here, so an entity
     * expiring this tick is still readable by every other system in the
     * same phase -- which is what lets a renderer draw its last frame.
     * The entity is destroyed rather than having its Lifetime removed,
     * because a lifetime is a statement about the entity and not about
     * one of its components; an app that wants the entity to outlive the
     * countdown gives it no Lifetime and counts down its own component.
     */
    class LifetimeSystem final : public ISystem
    {
    public:
        /**
         * @brief Age every entity with a Lifetime by one tick.
         * @param world The world to read lifetimes from and stage
         * decrements and destructions into.
         * @param tick The tick being processed; unused, since a
         * countdown depends only on what the entity has left.
         */
        void update(World &world, antwika::time::Tick tick) override;
    };

} // namespace antwika::ecs_commons
