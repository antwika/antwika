#pragma once

#include "antwika/holdem/Chips.hpp"
#include "antwika/holdem/SeatId.hpp"

namespace antwika::holdem
{

    /**
     * @brief Chips one seat is owed once a hand is over.
     */
    struct Payout
    {
        /**
         * @brief The seat being paid.
         */
        SeatId seat{};

        /**
         * @brief Chips returned to that seat's stack, summed across
         * every side pot it won a share of.
         */
        Chips amount{};

        bool operator==(const Payout &other) const = default;
    };

} // namespace antwika::holdem
