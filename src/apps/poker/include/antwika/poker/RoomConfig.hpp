#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

#include <antwika/holdem/Blinds.hpp>
#include <antwika/holdem/HandCategory.hpp>
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
