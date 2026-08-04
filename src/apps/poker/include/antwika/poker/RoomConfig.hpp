#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

#include <antwika/holdem/Blinds.hpp>
#include <antwika/holdem/HandCategory.hpp>

#include "antwika/poker/AgentStyle.hpp"
#include <antwika/holdem/Chips.hpp>

namespace antwika::poker
{

    using antwika::holdem::Blinds;
    using antwika::holdem::kHandCategoryCount;
    using antwika::holdem::Chips;

    /**
     * @brief How strongly the shipped agents rate each made hand.
     *
     * Indexed by HandCategory, weakest first, on a 0..100 scale.
     * A lookup rather than arithmetic on the enumeration, and a value
     * on RoomConfig rather than a constant in PolicyAgent.cpp, so the
     * config file can restate it at runtime; this is its shipped
     * default.
     */
    inline constexpr std::array<unsigned, kHandCategoryCount>
        kDefaultHandStrengths{20, 45, 62, 76, 85, 90, 95, 98, 100};

    /**
     * @brief When one style raises and when it merely calls.
     */
    struct AgentThresholds
    {
        /** @brief The strength at or above which it raises. */
        unsigned raiseAt = 0;

        /** @brief The strength at or above which it calls. */
        unsigned callAt = 0;

        /**
         * @brief Compare two thresholds.
         * @param other The thresholds to compare against.
         * @return True when both members match.
         */
        [[nodiscard]] bool operator==(
            const AgentThresholds &other) const = default;
    };

    /**
     * @brief How boldly each shipped style plays, in AgentStyle order.
     */
    /**
     * @brief How many seats the shipped table deals to at most.
     *
     * A cap on the styles table rather than on the table itself: a
     * seat past the list wraps round it, which is what "the styles
     * repeat round the table" means.
     */
    inline constexpr std::size_t kSeatStyleCount = 3;

    /**
     * @brief Which style sits in each seat, repeating round the table.
     */
    inline constexpr std::array<AgentStyle, kSeatStyleCount>
        kDefaultSeatStyles{
            AgentStyle::Balanced,
            AgentStyle::Tight,
            AgentStyle::Aggressive};

    inline constexpr std::array<AgentThresholds, kAgentStyleCount>
        kDefaultThresholds{
            AgentThresholds{.raiseAt = 80, .callAt = 55},
            AgentThresholds{.raiseAt = 70, .callAt = 40},
            AgentThresholds{.raiseAt = 55, .callAt = 25}};

    /**
     * @brief How a poker room is set up, before anybody walks in.
     */
    struct RoomConfig
    {
        /**
         * @brief Seats the table has.
         */
        std::size_t seatCount = 6;

        /**
         * @brief The blinds in force, which also fix the minimum bet.
         */
        Blinds blinds{.small = 5, .big = 10};

        /**
         * @brief Smallest stack a player may sit down with.
         */
        Chips minimumBuyIn = 100;

        /**
         * @brief The name every hand played here is written up under.
         */
        std::string tableName = "Antwika";

        /**
         * @brief Seed for the shuffle.
         *
         * Part of the room rather than of a recorded session: the same
         * seed and the same events deal the same cards, which is exactly
         * why no card ever has to be written into a replay.
         */
        std::uint64_t seed = 1;

        /**
         * @brief When each style raises and when it calls.
         *
         * Two numbers per style on hand strength's own 0..100 scale,
         * in AgentStyle's order; the decode refuses a style whose
         * raise threshold sits under its call threshold, since an
         * agent that raises hands it would not call is one nobody
         * meant.
         */
        std::array<AgentThresholds, kAgentStyleCount> thresholds =
            kDefaultThresholds;

        /**
         * @brief Which style sits in each seat, repeating round the
         * table.
         */
        std::array<AgentStyle, kSeatStyleCount> seatStyles =
            kDefaultSeatStyles;

        /**
         * @brief How strongly the agents rate each made hand.
         *
         * Weakest category first; the decode refuses a table that is
         * not non-decreasing, since a straight rated under a pair is a
         * table somebody wrote backwards.
         */
        std::array<unsigned, kHandCategoryCount> handStrengths =
            kDefaultHandStrengths;

        bool operator==(const RoomConfig &other) const = default;
    };

} // namespace antwika::poker
