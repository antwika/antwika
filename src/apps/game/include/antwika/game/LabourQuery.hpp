#pragma once

#include <cstdint>
#include <optional>

#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/World.hpp>

namespace antwika::game
{

    using antwika::ecs::World;

    /**
     * @file
     * @brief The read-only face of labour, for everything that is not
     * LabourSystem.
     *
     * **Published as a seam rather than left as a component read**, so a
     * cadence, a batch or a rating asks a function about an entity
     * instead of asking whether a component is there and then deciding
     * for itself what its absence meant. Every answer here is total: an
     * entity with no Workforce -- a building put up this tick, a road, a
     * handle whose entity is long dead -- answers the value the game had
     * before labour existed, which is **fully staffed**.
     *
     * That default is what let the goods chain be written and pass before
     * there were any people to allocate, and it is still what a building
     * the allocation has not reached yet behaves as.
     */

    /**
     * @brief How many people work at a building, out of how many it wants.
     *
     * A numerator and a denominator rather than a fraction, because a
     * fraction is the first floating-point number in the tick path and two
     * toolchains do not have to agree on where it rounds -- which is the
     * same argument DesirabilitySource is made of integers for.
     */
    struct Staffing
    {
        /** @brief How many people are at work there. */
        std::int32_t filled = 0;

        /** @brief How many the kind wants -- see workersWantedBy(). */
        std::int32_t wanted = 0;

        /**
         * @brief Compare two staffings.
         * @param other The staffing to compare against.
         * @return True when both numbers match.
         */
        [[nodiscard]] constexpr bool operator==(
            const Staffing &other) const = default;
    };

    /**
     * @brief Get how a building is staffed.
     *
     * **An entity with no Workforce answers fully staffed rather than
     * empty**, which is the rule this whole increment is written under:
     * an absent component means the value the game had before that
     * component existed. An entity with no Building at all wants nobody
     * and has nobody, which is the honest reading of a road or a walker.
     *
     * @param world Read as of its last commit().
     * @param entity The building to ask about; it need not be alive.
     * @return What it wants and what it has.
     */
    [[nodiscard]] Staffing staffingOf(
        const World &world, antwika::ecs::Entity entity);

    /**
     * @brief Get how many people work at a building.
     * @param world Read as of its last commit().
     * @param entity The building to ask about; it need not be alive.
     * @return staffingOf()'s numerator, on exactly its terms.
     */
    [[nodiscard]] std::int32_t workersAt(
        const World &world, antwika::ecs::Entity entity);

    /**
     * @brief Stretch a period out over however few people turned up.
     *
     * The one place staffing becomes a rate, and it is integer
     * arithmetic throughout: a building at half its complement takes
     * twice as long, one at a quarter takes four times as long, and one
     * with nobody at all does not work at all rather than taking
     * infinitely long.
     *
     * **Nothing rather than a very large number for the empty case**, so
     * a caller has to say what it does about it: a countdown held at a
     * huge value would eventually elapse, which is exactly the backlog
     * SpawnSystem and ProductionSystem hold their countdowns at zero to
     * avoid.
     *
     * @param period The period at a full complement.
     * @param staffing What the building wants and has.
     * @return The period to use, or nothing when nobody works there.
     */
    [[nodiscard]] constexpr std::optional<std::int32_t> workedPeriod(
        std::int32_t period, Staffing staffing) noexcept
    {
        if (staffing.wanted <= 0 || staffing.filled >= staffing.wanted)
        {
            return period;
        }

        if (staffing.filled <= 0)
        {
            return std::nullopt;
        }

        return period * staffing.wanted / staffing.filled;
    }

    static_assert(
        workedPeriod(10, Staffing{.filled = 0, .wanted = 0}) == 10);
    static_assert(
        workedPeriod(10, Staffing{.filled = 4, .wanted = 4}) == 10);
    static_assert(
        workedPeriod(10, Staffing{.filled = 2, .wanted = 4}) == 20);
    static_assert(
        !workedPeriod(10, Staffing{.filled = 0, .wanted = 4}).has_value());

} // namespace antwika::game
