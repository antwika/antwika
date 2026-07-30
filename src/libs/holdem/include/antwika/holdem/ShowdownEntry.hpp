#pragma once

#include <array>

#include "antwika/holdem/Card.hpp"
#include "antwika/holdem/HandValue.hpp"
#include "antwika/holdem/Limits.hpp"
#include "antwika/holdem/SeatId.hpp"

namespace antwika::holdem
{

    /**
     * @brief One seat's cards and their strength, as revealed at a
     * showdown.
     */
    struct ShowdownEntry
    {
        /**
         * @brief The seat that showed down.
         */
        SeatId seat{};

        /**
         * @brief The cards it turned over.
         */
        std::array<Card, kHoleCardCount> holeCards{};

        /**
         * @brief The best five-card hand those cards make with the
         * board.
         */
        HandValue value{};

        bool operator==(const ShowdownEntry &other) const = default;
    };

} // namespace antwika::holdem
