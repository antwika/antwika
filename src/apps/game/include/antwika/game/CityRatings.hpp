#pragma once

#include <cstdint>

#include <antwika/ecs/World.hpp>

namespace antwika::game
{

    using antwika::ecs::World;

    /**
     * @brief How the city is doing, as four numbers.
     *
     * **The first thing in this application that judges the city rather
     * than simulating it**, and everything about how it is written
     * follows from that. Every member is an integer, because a rating is
     * compared in GameSummary and a float from a division would not have
     * to round the same way on two toolchains. Nothing here is persisted,
     * because every member is a sum over things that already are. And no
     * event names any of it, because it is a pure function of the World
     * -- recording a rating would be recording the answer to a sum a
     * replay does over again anyway.
     *
     * GameState::score is deliberately not one of these. A score is fed
     * by an event and is a fact about what was dispatched; a rating is a
     * fact about what is standing.
     */
    struct CityRatings
    {
        /** @brief How many people live in the city. */
        std::int32_t population = 0;

        /**
         * @brief What share of the city's jobs are staffed, per cent.
         *
         * Zero when there are no jobs at all, which is what an empty
         * city and a city of nothing but houses both are.
         *
         * It reads staffingOf(), so a workplace the allocation has not
         * reached yet counts as staffed -- which is not a rounding of
         * the truth but a report of it, since that workplace really does
         * send its walkers and make its batches at full speed until
         * LabourSystem has said otherwise.
         */
        std::int32_t employment = 0;

        /**
         * @brief The mean housing tier, in hundredths of one.
         *
         * Hundredths rather than a fraction, for the reason every other
         * member is an integer. Zero when nobody has built a house, and
         * zero again when every house is still a tent -- the two say the
         * same thing about how well the city houses people.
         */
        std::int32_t averageHousingLevel = 0;

        /**
         * @brief What share of the services houses want reaches them,
         * per cent.
         *
         * Counted over every house and every Service rather than over
         * the ones a tier demands, so the number goes on rising once a
         * district has everything its houses currently ask for -- a
         * rating that stopped improving the moment a tier was satisfied
         * would tell a player nothing about where to build next.
         * Zero when there are no houses.
         */
        std::int32_t serviceReach = 0;

        /**
         * @brief Compare two sets of ratings.
         * @param other The ratings to compare against.
         * @return True when every member matches.
         */
        [[nodiscard]] constexpr bool operator==(
            const CityRatings &other) const = default;
    };

    /**
     * @brief Work out how the city is doing.
     *
     * A pure function of the World and of nothing else. The plan for this
     * increment also handed it the DesirabilityField; it is not a
     * parameter here because not one of the four members is a function of
     * it, and an argument a reader has to check the body for is worse than
     * one that is not offered.
     *
     * Every sum is commutative, so this reads ecs::View directly and the
     * answer is a function of the *set* of buildings rather than of the
     * order one was walked in -- which is the same argument
     * desirabilityFieldOf() is written under.
     *
     * @param world Read for the buildings, as of its last commit().
     * @return The four ratings.
     */
    [[nodiscard]] CityRatings ratingsOf(const World &world);

} // namespace antwika::game
