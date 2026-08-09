#pragma once

#include <array>
#include <cstdint>
#include <map>

#include <antwika/ecs/World.hpp>
#include <antwika/enums/Enumeration.hpp>

#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/GridExtent.hpp"

namespace antwika::game
{

    using antwika::ecs::World;

    struct DesirabilitySource final
    {
        std::int32_t contribution = 0;

        std::int32_t radius = 0;

        [[nodiscard]] constexpr bool operator==(
            const DesirabilitySource &other) const = default;
    };

    inline constexpr std::array<DesirabilitySource, kBuildingKindCount>
        kDesirabilityOf{{
            {.contribution = 0, .radius = 0},
            {.contribution = -1, .radius = 3},
            {.contribution = -3, .radius = 4},
            {.contribution = -2, .radius = 4},
            {.contribution = -2, .radius = 4},
            {.contribution = 2, .radius = 5},
            {.contribution = 1, .radius = 4},
            {.contribution = 2, .radius = 4},
            {.contribution = 1, .radius = 3},
            {.contribution = 1, .radius = 3},
        }};

    [[nodiscard]] constexpr DesirabilitySource desirabilityOf(
        BuildingKind kind) noexcept
    {
        return antwika::enums::pick(kDesirabilityOf, kind);
    }

    using DesirabilityField = std::map<Cell, std::int32_t>;

    [[nodiscard]] std::int32_t desirabilityFrom(
        DesirabilitySource source, std::int32_t distance) noexcept;

    [[nodiscard]] DesirabilityField desirabilityFieldOf(
        const World &world, GridExtent extent);

    [[nodiscard]] std::int32_t desirabilityAt(
        const DesirabilityField &field, Cell cell) noexcept;

    static_assert(desirabilityOf(BuildingKind::House).radius == 0);
    static_assert(desirabilityOf(BuildingKind::House).contribution == 0);

}
