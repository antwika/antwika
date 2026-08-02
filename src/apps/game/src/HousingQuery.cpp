#include "antwika/game/HousingQuery.hpp"

#include <cstdint>

#include <antwika/ecs/Entity.hpp>

#include "antwika/game/Household.hpp"

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

} // namespace antwika::game
