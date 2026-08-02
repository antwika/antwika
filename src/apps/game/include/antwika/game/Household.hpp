#pragma once

#include <cstdint>

#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/World.hpp>

#include "antwika/game/HousingLevel.hpp"

namespace antwika::game
{

    using antwika::ecs::World;

    /**
     * @brief Ticks between one person arriving or leaving and the next.
     *
     * One a second, so a fresh tent fills over about five seconds and a
     * district somebody has ruined empties at the same rate. Its own
     * constant rather than a share of kEvolvePeriodTicks, because a
     * house's occupancy and its tier are two decisions -- a city ought to
     * be able to fill faster than it grows, or the reverse -- and this
     * increment has no evidence for either.
     */
    inline constexpr std::int32_t kSettlerPeriodTicks = kTicksPerSecond;

    /**
     * @brief The people in one house, and how close it is to a change.
     *
     * **A component of its own rather than four more fields on
     * Building**, for Coverage's reason: every new component in this
     * increment is optional, and its absence is the value the game had
     * before that component existed. A house put up a moment ago has
     * none, and householdOf() answers a bottom-level, empty, freshly
     * counted-down one for it -- which is exactly what a save written
     * before housing existed means, so the format grows an optional
     * member and no migration.
     *
     * Both countdowns are here rather than being a modulus on the tick
     * number, for exactly the reason Building's three are: two houses put
     * up a tick apart would otherwise grow and shrink in lockstep for
     * ever, and a replay regenerates each countdown from the same click
     * that placed the house.
     */
    struct Household
    {
        /** @brief How well the household lives. */
        HousingLevel level = HousingLevel::Tent;

        /**
         * @brief Ticks of going on qualifying before the next level.
         *
         * Reset to the full period the moment the house stops meeting
         * the next level's requirements, rather than held where it was:
         * "has had this for a while" is the thing being measured, and a
         * countdown that remembered an interrupted stretch would measure
         * "has had this on and off".
         */
        std::int32_t ticksUntilEvolve = kEvolvePeriodTicks;

        /** @brief Ticks of going on falling short before the last one. */
        std::int32_t ticksUntilDevolve = kDevolvePeriodTicks;

        /**
         * @brief How many people live here.
         *
         * A level's capacity and a house's occupancy are one fact, so the
         * number belongs beside the level. What raises and lowers it is
         * PopulationSystem's rule, and populationCapacityOf() is the
         * ceiling it works against.
         */
        std::int32_t population = 0;

        /**
         * @brief Ticks until the next person arrives or leaves.
         *
         * A third countdown beside the two above it, and it is here for
         * their reason exactly: two houses put up a tick apart would
         * otherwise fill and empty in lockstep for ever.
         *
         * It is deliberately **not** reset when a house changes tier.
         * Growing into a bigger house is not a reason for the family
         * moving in to turn round at the door.
         */
        std::int32_t ticksUntilSettler = kSettlerPeriodTicks;

        /**
         * @brief Compare two households.
         * @param other The household to compare against.
         * @return True when every field matches.
         */
        [[nodiscard]] bool operator==(const Household &other) const
            = default;
    };

    /**
     * @brief Get the household living in a building.
     *
     * **Total, and an entity with no Household answers a default one
     * rather than throwing**, which is what makes an absent component
     * mean "a house nothing has happened to yet" rather than "unknown".
     * That is the shape every consumer of this header depends on.
     *
     * @param world Read as of its last commit(), like every other read.
     * @param entity The building to ask about; it need not be alive.
     * @return Its household, or a default one when it has none.
     */
    [[nodiscard]] Household householdOf(
        const World &world, antwika::ecs::Entity entity);

    /**
     * @brief Write a building's household, whether or not it had one.
     *
     * setCoverage()'s counterpart, and it exists for the same reason:
     * World::add() is staged and World::set() refuses an entity that has
     * no such component yet, so "add or set" is a decision every writer
     * would otherwise have to make for itself.
     *
     * @param world Staged into; the write lands at the next commit().
     * @param entity The building to write; must be alive.
     * @param household What is to live there.
     */
    void setHousehold(
        World &world, antwika::ecs::Entity entity, Household household);

} // namespace antwika::game
