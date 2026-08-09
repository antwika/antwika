#pragma once

#include <array>
#include <cstdint>

#include <antwika/enums/Enumeration.hpp>

#include "antwika/game/BuildingKind.hpp"

namespace antwika::game
{

    inline constexpr std::int64_t kRoadCost = 4;

    inline constexpr std::int64_t kRazeCost = 2;

    inline constexpr std::array<std::int64_t, kBuildingKindCount>
        kBuildingCosts{{
            10,
            40,
            40,
            60,
            70,
            50,
            15,
            60,
            70,
            50,
        }};

    [[nodiscard]] constexpr std::int64_t costOf(BuildingKind kind) noexcept
    {
        return antwika::enums::pick(kBuildingCosts, kind);
    }

    static_assert(costOf(BuildingKind::House) == 10);

    static_assert(
        []
        {
            for (const auto cost : kBuildingCosts)
            {
                if (cost <= 0)
                {
                    return false;
                }
            }

            return true;
        }());

}
