#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/World.hpp>

#include "antwika/game/BuildingKind.hpp"

namespace antwika::game
{

    using antwika::ecs::World;

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

    /**
     * @brief How many of the city's people work at one building.
     *
     * **A component of its own rather than a field on Building**, for
     * Coverage's and Household's reason: every new component in this
     * increment is optional, and its absence is the value the game had
     * before the component existed -- which here is "fully staffed", so a
     * city whose labour has never been allocated behaves exactly as it
     * did before there were any people to allocate. See staffingOf().
     *
     * **Only what was allocated is stored, never what was wanted.** How
     * many workers a kind wants is workersWantedBy(), a table, and a copy
     * of it on the component would be a second truth that could disagree
     * with the kind standing on the cell -- the same argument
     * footprintOf() and kDesirabilityOf are tables for.
     */
    struct Workforce
    {
        /** @brief How many people the city has put to work here. */
        std::int32_t employed = 0;

        /**
         * @brief Compare two workforces.
         * @param other The workforce to compare against.
         * @return True when the same number of people work at both.
         */
        [[nodiscard]] bool operator==(const Workforce &other) const
            = default;
    };

    /**
     * @brief Write a building's workforce, whether or not it had one.
     *
     * setCoverage()'s and setHousehold()'s counterpart, and it exists for
     * their reason: World::add() is staged and World::set() refuses an
     * entity that has no such component yet, so "add or set" is a
     * decision every writer would otherwise have to make for itself.
     *
     * @param world Staged into; the write lands at the next commit().
     * @param entity The building to write; must be alive.
     * @param workforce Who works there.
     */
    void setWorkforce(
        World &world, antwika::ecs::Entity entity, Workforce workforce);

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
