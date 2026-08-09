#pragma once

#include <optional>

#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/World.hpp>

#include "antwika/game/BuildingIndex.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/PathIndex.hpp"

namespace antwika::game
{

    struct Journey final
    {
        Cell towards{};

        antwika::ecs::Entity house = antwika::ecs::kNullEntity;

        [[nodiscard]] bool operator==(const Journey &other) const = default;
    };

    [[nodiscard]] std::optional<Cell> nearestGate(
        Cell from, const BuildingIndex &built, GridExtent extent);

    [[nodiscard]] antwika::ecs::Entity nearestVacancy(
        const antwika::ecs::World &world,
        Cell from,
        antwika::ecs::Entity leaving,
        const BuildingIndex &built,
        GridExtent extent);

}
