#pragma once

#include "antwika/holdem/Chips.hpp"
#include "antwika/holdem/TableMemory.hpp"

namespace antwika::holdem
{

    /**
     * @brief The live bet on one street, and what may legally beat it.
     *
     * Holds the two numbers a round of betting is: what it costs to stay
     * in, and how much a raise has to add to reopen the betting.
     * Knows nothing about seats, stacks, pots or stages -- a caller
     * hands it the one figure each question needs and acts on the
     * answer.
     *
     * Split out of Table, where the same two numbers were visible to the
     * dealing and showdown code that has no business reading them.
     */
    class BettingRound final
    {
    public:
        /**
         * @brief Open the pre-flop round, where the big blind is live.
         * @param bigBlind The forced bet, which is also the minimum
         * raise.
         */
        void open(Chips bigBlind) noexcept;

        /**
         * @brief Start a later street, where nothing is live yet.
         *
         * The minimum raise stays the big blind, which is what makes the
         * first bet on a street cost at least that.
         *
         * @param bigBlind The forced bet the minimum raise falls back
         * to.
         */
        void reset(Chips bigBlind) noexcept;

        /**
         * @brief End the round because the hand has ended.
         *
         * Clears the live bet and leaves the minimum raise alone: the
         * next hand opens with open(), which sets both.
         */
        void close() noexcept;

        /**
         * @brief Read what it costs to stay in.
         * @return The largest amount committed by anybody this round.
         */
        [[nodiscard]] Chips bet() const noexcept;

        /**
         * @brief Check whether there is anything to call.
         * @return True when somebody has bet this round.
         */
        [[nodiscard]] bool isLive() const noexcept;

        /**
         * @brief Read the smallest full raise.
         * @return The total a raiser has to reach to reopen the betting.
         */
        [[nodiscard]] Chips minimumRaiseTo() const noexcept;

        /**
         * @brief Work out what one player still owes.
         * @param roundCommitted What that player has put in this round.
         * @return The difference, which is zero for a covered player.
         */
        [[nodiscard]] Chips owedBy(Chips roundCommitted) const noexcept;

        /**
         * @brief Check whether one player owes nothing.
         * @param roundCommitted What that player has put in this round.
         * @return True when they have matched the live bet.
         */
        [[nodiscard]] bool isCovered(Chips roundCommitted) const noexcept;

        /**
         * @brief Raise the live bet, having checked that it may be.
         *
         * Checks only what the round itself knows about. Whether the
         * player may raise at all, and whether they hold the chips, are
         * the caller's to check first.
         *
         * @param target The total this player is staking this round.
         * @param allInTo The total that would leave them with no chips,
         * which is the one target allowed below the minimum.
         * @return True when it was a full raise, which reopens the
         * betting for everybody who has already acted.
         * @throws IllegalActionError If target does not beat the live
         * bet, or falls below the minimum while holding chips back.
         */
        [[nodiscard]] bool raiseTo(Chips target, Chips allInTo);

        /**
         * @brief Take the round's standing, as a value.
         * @return The live bet and the last full raise.
         */
        [[nodiscard]] BettingMemory remember() const noexcept;

        /**
         * @brief Stand this round at a remembered position.
         * @param memory The position to stand at.
         */
        void restore(const BettingMemory &memory) noexcept;

    private:
        Chips currentBet = 0;
        Chips lastRaiseSize = 0;
    };

} // namespace antwika::holdem
