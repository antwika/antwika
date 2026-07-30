#pragma once

#include <array>
#include <cstddef>
#include <vector>

#include "antwika/holdem/Blinds.hpp"
#include "antwika/holdem/Card.hpp"
#include "antwika/holdem/Chips.hpp"
#include "antwika/holdem/Limits.hpp"
#include "antwika/holdem/SeatId.hpp"
#include "antwika/holdem/Stage.hpp"

namespace antwika::holdem
{

    /**
     * @brief Everything one player may see when it is their turn.
     *
     * A snapshot rather than a reference to the table, so an agent
     * cannot reach past what a player at a real table would know -- most
     * of all, nobody else's hole cards.
     */
    struct TableView
    {
        /**
         * @brief The seat being asked to act.
         */
        SeatId seat{};

        /**
         * @brief How far the hand has progressed.
         */
        Stage stage{};

        /**
         * @brief This seat's own two cards.
         */
        std::array<Card, kHoleCardCount> holeCards{};

        /**
         * @brief The community cards turned over so far.
         */
        std::vector<Card> board;

        /**
         * @brief Chips already in the middle, all rounds included.
         */
        Chips pot{};

        /**
         * @brief Chips left in front of this seat.
         */
        Chips stack{};

        /**
         * @brief The largest amount any seat has staked this round.
         */
        Chips currentBet{};

        /**
         * @brief Chips a call would cost, already capped at the stack --
         * so a call is affordable by construction, and zero means a
         * check is the free option.
         */
        Chips toCall{};

        /**
         * @brief Smallest legal Bet/Raise amount under the rules.
         *
         * May exceed maxRaiseTo on a short stack, in which case the only
         * legal raise is all-in for maxRaiseTo.
         */
        Chips minRaiseTo{};

        /**
         * @brief Largest legal Bet/Raise amount: all-in.
         *
         * A raise is impossible at all when this does not exceed
         * currentBet.
         */
        Chips maxRaiseTo{};

        /**
         * @brief Whether raising is on offer at all.
         *
         * False once somebody has gone all-in for less than a full raise
         * behind a bet this seat had already matched: the difference can
         * be called, but the betting was not reopened.
         */
        bool mayRaise = true;

        /**
         * @brief How many players have not folded, this seat included.
         */
        std::size_t playersInHand{};

        /**
         * @brief The blinds in force, which also set the minimum bet.
         */
        Blinds blinds{};

        bool operator==(const TableView &other) const = default;
    };

} // namespace antwika::holdem
