#include "antwika/ecs_commons/LifetimeSystem.hpp"

#include "antwika/ecs_commons/Lifetime.hpp"

namespace antwika::ecs_commons
{

    void LifetimeSystem::update(World &world, antwika::time::Tick)
    {
        for (const auto entity : world.view<Lifetime>())
        {
            const auto lifetime = world.get<Lifetime>(entity);
            if (lifetime.remaining <= 1)
            {
                world.destroy(entity);
                continue;
            }

            world.set<Lifetime>(
                entity, Lifetime{.remaining = lifetime.remaining - 1});
        }
    }

} // namespace antwika::ecs_commons
