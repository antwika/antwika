#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "antwika/game/BuildingKind.hpp"

namespace antwika::game
{

    /**
     * @brief How many of the city's people one kind of building needs to
     * work at full speed.
     *
     * A table keyed by kind for footprintOf()'s reason: a ghost, a rating
     * and a system all have to agree about a kind before any entity of it
     * exists, which a component cannot tell them.
     *
     * **A house wants nobody and neither does a storehouse**, and the two
     * zeroes are there for different reasons. Nobody works at the place
     * they live in. And nothing in this increment reads a storehouse's
     * staffing -- it sends no walker and makes no batch -- so a demand
     * there would take people off buildings that do something with them,
     * with no effect a player could see to explain where they went.
     *
     * @param kind The kind to ask about.
     * @return How many workers it wants, never negative.
     */
    [[nodiscard]] constexpr std::int32_t workersWantedBy(
        BuildingKind kind) noexcept
    {
        constexpr std::array<std::int32_t, kBuildingKindCount> wanted{
            0,  // House
            4,  // Farm
            4,  // ClayPit
            4,  // Workshop
            0,  // Storage
            3,  // Market
            1,  // Well
            2,  // Doctor
            2,  // FireStation
            2,  // EngineerPost
        };

        return wanted[buildingKindIndex(kind) % kBuildingKindCount];
    }

    // A kind wants workers exactly when staffing it changes something.
    // In round one that is the walker it sends and the batch it makes.
    // And every kind that makes a batch also sends somebody.
    // So the two lists have to be the same list.
    static_assert(
        []
        {
            for (std::size_t index = 0; index < kBuildingKindCount; ++index)
            {
                const auto kind = static_cast<BuildingKind>(index);

                if ((workersWantedBy(kind) > 0) != sendsWalkers(kind))
                {
                    return false;
                }
            }

            return true;
        }(),
        "a kind wants workers exactly when it sends somebody out");

    static_assert(workersWantedBy(BuildingKind::House) == 0);
    static_assert(workersWantedBy(BuildingKind::Storage) == 0);
    static_assert(workersWantedBy(BuildingKind::Well) > 0);

} // namespace antwika::game
