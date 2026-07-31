#pragma once

#include <array>
#include <cstddef>

#include <antwika/rng/IRng.hpp>

#include "antwika/holdem/Card.hpp"
#include "antwika/holdem/IDeck.hpp"

namespace antwika::holdem
{

    /**
     * @brief A 52-card deck, shuffled by Fisher-Yates over an rng::IRng.
     *
     * Fisher-Yates by hand rather than std::shuffle, whose card order is
     * unspecified and does differ between standard library
     * implementations: the shuffle has to be part of what a replay
     * reproduces, so it has to be spelled out here.
     */
    class Deck final : public IDeck
    {
    public:
        /**
         * @brief Construct an unshuffled deck over its bit source.
         *
         * The deck starts in ascending card order; call shuffle() before
         * dealing.
         * @param rng Draws the swap positions for every shuffle().
         */
        explicit Deck(rng::IRng &rng);

        /**
         * @brief Return every card to the deck and re-order it.
         */
        void shuffle() override;

        /**
         * @brief Take the next card off the top.
         * @return The dealt card.
         * @throws DeckExhaustedError If all 52 cards are already dealt.
         */
        [[nodiscard]] Card deal() override;

        /**
         * @brief Count the cards left to deal.
         * @return How many cards remain before the next shuffle().
         */
        [[nodiscard]] std::size_t remaining() const noexcept;

    private:
        rng::IRng &rng;
        std::array<Card, kCardCount> cards{};
        std::size_t dealt = 0;
    };

} // namespace antwika::holdem
