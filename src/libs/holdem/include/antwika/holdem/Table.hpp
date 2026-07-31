#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#include "antwika/holdem/Action.hpp"
#include "antwika/holdem/BettingRound.hpp"
#include "antwika/holdem/Blinds.hpp"
#include "antwika/holdem/Card.hpp"
#include "antwika/holdem/Chips.hpp"
#include "antwika/holdem/HandFlow.hpp"
#include "antwika/holdem/HandResult.hpp"
#include "antwika/holdem/HandValue.hpp"
#include "antwika/holdem/IDeck.hpp"
#include "antwika/holdem/Seat.hpp"
#include "antwika/holdem/SeatId.hpp"
#include "antwika/holdem/ShowdownEntry.hpp"
#include "antwika/holdem/Stage.hpp"
#include "antwika/holdem/TableView.hpp"

namespace antwika::holdem
{

    /**
     * @brief One no-limit hold'em table: who is seated, whose turn it
     * is, and what a legal turn even is.
     *
     * Holds no opinion about how to play and asks nobody anything --
     * every decision arrives through apply(), which is the only way the
     * hand moves. That keeps the rules in one testable place and lets
     * TableRunner, an agent, a replayed event or a test drive the very
     * same state machine.
     *
     * A hand advances itself as far as it can between decisions: dealing
     * the flop, turn and river, running out the board when nobody has
     * chips left to bet with, and paying out at the end all happen
     * inside apply(). So while isHandInProgress() holds, seatToAct()
     * always names somebody.
     *
     * The betting rules, the stage progression and the showdown live in
     * BettingRound, HandFlow and Showdown.hpp; what stays here is the
     * coordination between them, and the seats.
     *
     * **The seats stay here deliberately, and this was decided rather
     * than left.** Twenty-three of this class's methods touch the seat
     * vector, across seating, dealing, betting and payout alike, and
     * most of them mutate a Seat: blinds move chips, a call moves
     * chips, a pot pays them back. A Seating class owning the vector
     * would therefore have to hand a mutable Seat & to all three, at
     * which point it is a std::vector<Seat> with a bounds check and
     * nothing has been decoupled. The seats are not a fifth
     * responsibility -- they are the substrate the other four
     * coordinate through, which is why they belong to the coordinator.
     */
    class Table final
    {
    public:
        /**
         * @brief Construct an empty table.
         * @param seatCount How many seats it has, in
         * [kMinSeats, kMaxSeats].
         * @param blinds The forced bets, which also fix the minimum bet.
         * @throws TableStateError If seatCount is out of range.
         */
        Table(std::size_t seatCount, Blinds blinds);

        Table(const Table &) = delete;
        Table(Table &&) = delete;

        Table &operator=(const Table &) = delete;
        Table &operator=(Table &&) = delete;

        /**
         * @brief Count this table's seats, occupied or not.
         * @return The seat count fixed at construction.
         */
        [[nodiscard]] std::size_t seatCount() const noexcept;

        /**
         * @brief Get the blinds in force.
         * @return The blinds fixed at construction.
         */
        [[nodiscard]] Blinds blinds() const noexcept;

        /**
         * @brief Read one seat's chips, cards and standing.
         * @param seat The seat to read.
         * @return That seat's current state.
         * @throws TableStateError If seat is out of range.
         */
        [[nodiscard]] const Seat &seatAt(SeatId seat) const;

        /**
         * @brief Find a seat nobody is sitting in.
         * @return The lowest free seat, or nothing if the table is full.
         */
        [[nodiscard]] std::optional<SeatId> firstFreeSeat() const;

        /**
         * @brief Sit a player down with a stack of chips.
         *
         * Allowed mid-hand: the new seat simply sits out until the next
         * hand is dealt.
         * @param seat The seat to fill.
         * @param stack Chips the player brings to the table.
         * @throws TableStateError If seat is out of range or occupied.
         */
        void seatPlayer(SeatId seat, Chips stack);

        /**
         * @brief Add chips to a seated player's stack between hands.
         * @param seat The seat to top up.
         * @param amount Chips to add.
         * @throws TableStateError If seat is out of range, empty, or in
         * the current hand.
         */
        void addChips(SeatId seat, Chips amount);

        /**
         * @brief Empty a seat.
         *
         * Refused while the hand this seat has chips in is still being
         * played, whether or not it has folded: a folded player's stake
         * is still in the pot, and clearing the seat would delete it
         * from the contributions the side pots are built from.
         * A seat that has staked nothing may leave at any time.
         * @param seat The seat to clear; already-empty is fine.
         * @throws TableStateError If seat is out of range, or a hand is
         * in progress and its player has chips in the pot.
         */
        void removePlayer(SeatId seat);

        /**
         * @brief Check whether a hand could be dealt right now.
         * @return True if no hand is running and at least kMinSeats
         * seated players still have chips.
         */
        [[nodiscard]] bool canStartHand() const noexcept;

        /**
         * @brief Move the button along, post the blinds and deal.
         *
         * The button moves to the next seated player with chips, so
         * players who busted or sat down mid-hand are accounted for
         * before positions are decided. Heads-up, the button posts the
         * small blind.
         * @param deck Shuffled here, then dealt from for the whole hand;
         * it must outlive the hand.
         * @throws TableStateError If a hand is already running, or fewer
         * than kMinSeats seated players have chips.
         */
        void startHand(IDeck &deck);

        /**
         * @brief Check whether a hand is being played.
         * @return True between startHand() and the action that ends the
         * hand.
         */
        [[nodiscard]] bool isHandInProgress() const noexcept;

        /**
         * @brief Get how far the current or last hand progressed.
         * @return The current stage.
         */
        [[nodiscard]] Stage stage() const noexcept;

        /**
         * @brief Get the dealer button's seat.
         * @return The seat holding the button.
         */
        [[nodiscard]] SeatId button() const noexcept;

        /**
         * @brief Read the community cards.
         * @return The board as dealt so far.
         */
        [[nodiscard]] const std::vector<Card> &board() const noexcept;

        /**
         * @brief Count chips in the middle.
         *
         * Zero once a hand has been paid out; lastResult() is what
         * reports the pot a finished hand was played for.
         * @return The pot as it stands.
         */
        [[nodiscard]] Chips pot() const noexcept;

        /**
         * @brief Find whose turn it is.
         * @return The seat that must act, or nothing when no hand is in
         * progress.
         */
        [[nodiscard]] std::optional<SeatId> seatToAct() const noexcept;

        /**
         * @brief Take a snapshot of what one seat may see.
         * @param seat The seat to build the view for, normally
         * seatToAct().
         * @return That seat's view of the hand.
         * @throws TableStateError If seat is out of range.
         */
        [[nodiscard]] TableView viewFor(SeatId seat) const;

        /**
         * @brief Apply the decision of whichever seat is to act.
         *
         * Advances the hand as far as it can afterwards, which may deal
         * the next street, run the board out, or finish the hand and pay
         * it out.
         * @param action What that seat chose to do.
         * @throws TableStateError If no seat is waiting to act.
         * @throws IllegalActionError If the action breaks the betting
         * rules -- checking into a bet, calling with nothing to call,
         * betting into a live bet, raising below the minimum with chips
         * to spare, staking more than the stack holds, raising after an
         * all-in that did not reopen the betting, or naming an action
         * type that is not one of the five.
         */
        void apply(Action action);

        /**
         * @brief Read what happened in the last finished hand.
         *
         * Keeps reporting that hand while the next one is being played,
         * so a caller narrating a session never has to catch it up
         * before the deal.
         * @return The last finished hand's result.
         * @throws TableStateError If no hand has finished yet.
         */
        [[nodiscard]] const HandResult &lastResult() const;

        /**
         * @brief Count the hands dealt at this table.
         * @return How many times startHand() has succeeded.
         */
        [[nodiscard]] std::uint64_t handsPlayed() const noexcept;

    private:
        std::vector<Seat> seats;
        Blinds blindLevels;
        std::optional<HandResult> result;
        std::optional<SeatId> toAct;
        Chips potChips = 0;
        BettingRound betting;
        HandFlow flow;
        std::uint64_t handCount = 0;
        SeatId buttonSeat{};
        bool handInProgress = false;

        void requireSeatInRange(SeatId seat) const;
        [[nodiscard]] std::size_t countInHand() const noexcept;
        [[nodiscard]] std::size_t countAbleToAct() const noexcept;
        void commit(Seat &seat, Chips amount) noexcept;
        void applyCall(Seat &seat);
        void applyRaise(SeatId actor, Chips target);
        void dealHoleCards();
        void postBlinds();
        void openBetting(SeatId from);
        void advanceAfterAction(SeatId actor);
        void resetBettingRound() noexcept;
        void closeRound();
        void finishWithoutShowdown();
        void finishWithShowdown();
        void finishHand(
            const std::vector<HandValue> &values,
            std::vector<ShowdownEntry> entries);
    };

} // namespace antwika::holdem
