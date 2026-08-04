#pragma once

#include <functional>
#include <optional>
#include <vector>

#include "antwika/holdem/Card.hpp"
#include "antwika/holdem/IDeck.hpp"
#include "antwika/holdem/Stage.hpp"

namespace antwika::holdem
{

    /**
     * @brief How far one hand has got, what is on the board, and where
     * the cards come from.
     *
     * The three travel together because a street is all of them at once:
     * the stage says how many cards the next street turns over, the deck
     * supplies them and the board keeps them. Splitting them would mean
     * a caller bumping the stage and then remembering how many cards
     * that stage owes.
     *
     * Knows nothing about seats, chips or whose turn it is -- it deals
     * cards and says what stage the hand is on, and a caller decides who
     * gets them.
     *
     * Split out of Table, where the deal was reachable from the betting
     * and seating code that has no business dealing.
     */
    class HandFlow final
    {
    public:
        /**
         * @brief Start a hand: take the deck, shuffle it and clear the
         * board.
         * @param source Dealt from for the whole hand; it must outlive
         * the hand.
         */
        void begin(IDeck &source);

        /**
         * @brief Let go of the deck at the end of the hand.
         *
         * The stage and the board stay as they were, because a finished
         * hand is still asked what it came to.
         */
        void end() noexcept;

        /**
         * @brief Stand the flow at a remembered stage and board.
         *
         * State only: the deck stays let go of, which is what a table
         * between hands looks like.
         * A hand in progress calls adopt() as well.
         *
         * @param stage How far the hand had progressed.
         * @param board The community cards turned so far.
         */
        void resume(Stage stage, std::vector<Card> board);

        /**
         * @brief Rebind the deck a resumed hand deals from.
         *
         * begin()'s taking of the deck without its shuffling and
         * clearing: a restored mid-hand flow deals its next card off
         * the deck exactly where the remembered one stood.
         *
         * @param source The deck to deal the rest of the hand from.
         * Must outlive the hand, exactly as begin()'s must.
         */
        void adopt(IDeck &source) noexcept;

        /**
         * @brief Get how far the current or last hand progressed.
         * @return The current stage.
         */
        [[nodiscard]] Stage stage() const noexcept;

        /**
         * @brief Read the community cards.
         * @return The board as dealt so far.
         */
        [[nodiscard]] const std::vector<Card> &board() const noexcept;

        /**
         * @brief Check whether a street is still to come.
         * @return True before the river is out, false once it is and at
         * the showdown.
         */
        [[nodiscard]] bool hasStreetToDeal() const noexcept;

        /**
         * @brief Take one card off the deck, for a player rather than
         * the board.
         * @return The dealt card.
         * @throws TableStateError If no hand has begun.
         * @throws DeckExhaustedError If the deck ran out.
         */
        [[nodiscard]] Card dealCard();

        /**
         * @brief Move to the next street and turn its cards over.
         *
         * Three on the flop and one on every street after, which is why
         * the stage moves first and the count follows from it.
         * @throws TableStateError If no hand has begun, or the river is
         * already out.
         * @throws DeckExhaustedError If the deck ran out.
         */
        void dealStreet();

        /**
         * @brief Reach the showdown, where cards get compared.
         *
         * Deals nothing: the board a showdown compares against is
         * already out.
         */
        void toShowdown() noexcept;

    private:
        std::vector<Card> communityCards;
        // The one borrowed collaborator arriving after construction.
        // A reference member cannot express that, nor the letting go.
        // An optional reference says "none yet" without a raw pointer.
        std::optional<std::reference_wrapper<IDeck>> deck;
        Stage currentStage = Stage::PreFlop;

        [[nodiscard]] IDeck &requireDeck() const;
    };

} // namespace antwika::holdem
