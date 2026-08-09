#pragma once

#include <cstdint>

#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/World.hpp>

#include "antwika/game/HousingLevel.hpp"

namespace antwika::game
{

    using antwika::ecs::World;

    [[nodiscard]] HousingLevel levelOf(
        const World &world, antwika::ecs::Entity entity);

    [[nodiscard]] constexpr std::int32_t populationCapacityOf(
        HousingLevel level) noexcept
    {
        return requirementOf(level).populationCapacity;
    }

    [[nodiscard]] std::int32_t populationAt(
        const World &world, antwika::ecs::Entity entity);

    [[nodiscard]] std::int32_t stockCapacityAt(
        const World &world,
        antwika::ecs::Entity entity,
        BuildingKind kind);

    static_assert(
        populationCapacityOf(HousingLevel::Tent)
        < populationCapacityOf(HousingLevel::Cottage));

}
