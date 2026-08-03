#pragma once

#include <array>
#include <cstdint>

#include "antwika/game/BuildingKind.hpp"

namespace antwika::game
{

    /**
     * @brief What laying one cell of road takes out of the bank.
     *
     * Charged per cell actually laid rather than per cell of a plan: a
     * route through existing road lays nothing on those cells, so it
     * takes nothing for them either. A dragged run of road therefore
     * costs exactly this times the tiles it put down, which is the one
     * rule a plain click and a drag share.
     */
    inline constexpr std::int64_t kRoadCost = 4;

    /**
     * @brief What putting up each kind of building takes out of the
     * bank.
     *
     * A table keyed by kind rather than a field anywhere, for
     * kFootprints' reason exactly: a copy on a component could disagree
     * with the kind standing on the cell, and BuildingKind's values are
     * contiguous precisely so a kind can index a table.
     */
    inline constexpr std::array<std::int64_t, kBuildingKindCount>
        kBuildingCosts{{
            10,  // House
            40,  // Farm
            40,  // ClayPit
            60,  // Workshop
            70,  // Storage
            50,  // Market
            15,  // Well
            60,  // Doctor
            70,  // FireStation
            50,  // EngineerPost
        }};

    /**
     * @brief Get what putting up a building of this kind costs.
     * @param kind The kind to price.
     * @return Its cost, always positive.
     */
    [[nodiscard]] constexpr std::int64_t costOf(BuildingKind kind) noexcept
    {
        return kBuildingCosts[buildingKindIndex(kind) % kBuildingKindCount];
    }

    static_assert(costOf(BuildingKind::House) == 10);

    // Nothing here is free.
    // A kind that cost nothing would silently spare its placements.
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

} // namespace antwika::game
