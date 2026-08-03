#include "antwika/game/HousingQuery.hpp"

#include <cstdint>

#include <antwika/ecs/Entity.hpp>

#include "antwika/game/Household.hpp"
#include "antwika/game/Store.hpp"

namespace antwika::game
{

    HousingLevel levelOf(const World &world, antwika::ecs::Entity entity)
    {
        return householdOf(world, entity).level;
    }

    std::int32_t populationAt(
        const World &world, antwika::ecs::Entity entity)
    {
        return householdOf(world, entity).population;
    }

    std::int32_t stockCapacityAt(
        const World &world,
        antwika::ecs::Entity entity,
        BuildingKind kind)
    {
        return housesPeople(kind)
            ? stockCapacityOf(levelOf(world, entity))
            : capacityOf(kind);
    }

} // namespace antwika::game
