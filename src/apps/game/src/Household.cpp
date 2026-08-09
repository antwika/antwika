#include "antwika/game/Household.hpp"

#include <antwika/ecs/Entity.hpp>

namespace antwika::game
{

    Household householdOf(const World &world, antwika::ecs::Entity entity)
    {
        if (!world.has<Household>(entity))
        {
            return Household{};
        }

        return world.get<Household>(entity);
    }

    void setHousehold(
        World &world, antwika::ecs::Entity entity, Household household)
    {
        if (world.has<Household>(entity))
        {
            world.set<Household>(entity, household);
            return;
        }

        world.add<Household>(entity, household);
    }

}
