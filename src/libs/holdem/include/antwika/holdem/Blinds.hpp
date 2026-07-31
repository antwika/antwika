#pragma once

#include "antwika/holdem/Chips.hpp"

namespace antwika::holdem
{

    /**
     * @brief The two forced bets that start every hand, and with them
     * the smallest legal opening bet.
     *
     * A plain aggregate with no invariant of its own, so that a caller
     * can name one with designated initialisers and compare two with
     * ==.
     * The one rule -- that small never exceeds big -- is checked by
     * Table's constructor, which is the only door these reach the
     * betting rules through, and the only place that knows what a
     * violation would do to a pot.
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
