#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include <antwika/holdem/Blinds.hpp>
#include <antwika/holdem/Chips.hpp>

namespace antwika::poker
{

    using antwika::holdem::Blinds;
    using antwika::holdem::Chips;

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

        bool operator==(const RoomConfig &other) const = default;
    };

} // namespace antwika::poker
