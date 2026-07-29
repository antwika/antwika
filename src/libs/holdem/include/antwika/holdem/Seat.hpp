#pragma once

#include <array>

#include "antwika/holdem/Card.hpp"
#include "antwika/holdem/Chips.hpp"
#include "antwika/holdem/Limits.hpp"

namespace antwika::holdem
{

    /**
     * @brief One seat's chips, cards and standing in the current hand.
     */
    struct Seat
    {
        /**
         * @brief Chips in front of this seat, not yet wagered.
         */
        Chips stack{};

        /**
         * @brief Chips this seat has put in across the whole hand, which
         * is what side pots are carved out of.
         */
        Chips committed{};

        /**
         * @brief Chips this seat has put in during the current betting
         * round, which is what a call has to match.
         */
        Chips roundCommitted{};

        /**
         * @brief Whether a player occupies this seat at all.
         */
        bool occupied = false;

        /**
         * @brief Whether this seat was dealt in and has not folded.
         */
        bool inHand = false;

        /**
         * @brief Whether this seat has already had its turn since the
         * last bet or raise.
         *
         * What makes the big blind's option work: it has chips matching
         * the current bet from the moment it posts, so only "has not
         * acted yet" keeps the round open for it.
         */
        bool actedThisRound = false;

        /**
         * @brief Whether this seat is still allowed to raise this round.
         *
         * Cleared for everyone who has already acted when somebody goes
         * all-in for less than a full raise: that all-in owes them a
         * chance to call it, but not a fresh chance to re-raise.
         */
        bool mayRaise = true;

        /**
         * @brief This seat's private cards, meaningful only while
         * inHand.
         */
        std::array<Card, kHoleCardCount> holeCards{};

        bool operator==(const Seat &other) const = default;
    };

} // namespace antwika::holdem
