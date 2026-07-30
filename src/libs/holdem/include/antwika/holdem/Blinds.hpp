#pragma once

#include "antwika/holdem/Chips.hpp"

namespace antwika::holdem
{

    /**
     * @brief The two forced bets that start every hand, and with them
     * the smallest legal opening bet.
     */
    struct Blinds
    {
        /**
         * @brief Posted by the seat left of the button, or by the button
         * itself when only two players are in the hand.
         */
        Chips small{};

        /**
         * @brief Posted by the next seat along, and also the minimum a
         * player may bet or raise by.
         */
        Chips big{};

        bool operator==(const Blinds &other) const = default;
    };

} // namespace antwika::holdem
