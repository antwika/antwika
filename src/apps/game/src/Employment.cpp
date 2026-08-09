#include "antwika/game/Employment.hpp"

#include <cstdint>

#include <antwika/ecs/Entity.hpp>

namespace antwika::game
{

    std::int32_t employedCount(const Employment &employment)
    {
        std::int32_t total = 0;

        for (const auto &holding : employment.jobs)
        {
            total += holding.count;
        }

        return total;
    }

    void setEmployment(
        World &world,
        antwika::ecs::Entity entity,
        const Employment &employment)
    {
        if (world.has<Employment>(entity))
        {
            world.set<Employment>(entity, employment);
            return;
        }

        world.add<Employment>(entity, employment);
    }

}
