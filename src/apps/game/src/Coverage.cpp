#include "antwika/game/Coverage.hpp"

#include <cstdint>

#include <antwika/ecs/Entity.hpp>

namespace antwika::game
{

    Coverage coverageOf(const World &world, antwika::ecs::Entity entity)
    {
        if (!world.has<Coverage>(entity))
        {
            return Coverage{};
        }

        return world.get<Coverage>(entity);
    }

    std::int32_t coverageOf(
        const World &world, antwika::ecs::Entity entity, Service service)
    {
        return coverageOf(world, entity).ticksLeft[serviceIndex(service)];
    }

    void setCoverage(
        World &world, antwika::ecs::Entity entity, Coverage coverage)
    {
        if (world.has<Coverage>(entity))
        {
            world.set<Coverage>(entity, coverage);
            return;
        }

        world.add<Coverage>(entity, coverage);
    }

} // namespace antwika::game
