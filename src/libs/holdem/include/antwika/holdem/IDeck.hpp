#pragma once

#include "antwika/holdem/Card.hpp"

namespace antwika::holdem
{

    /**
     * @brief Source of cards for one hand at a time.
     *
     * Table deals through this rather than owning a Deck so a test can
     * hand it a known card order and assert on an exact showdown --
     * without that seam, every board would have to be reached by
     * searching for a seed that produces it.
     */
    class IDeck
    {
    public:
        virtual ~IDeck() = default;

        /**
         * @brief Return every card to the deck and re-order it.
         *
         * Called once before each hand is dealt.
         */
        virtual void shuffle() = 0;

        /**
         * @brief Take the next card off the top.
         * @return The dealt card, which this deck will not deal again
         * before the next shuffle().
         * @throws DeckExhaustedError If no cards remain.
         */
        [[nodiscard]] virtual Card deal() = 0;
    };

} // namespace antwika::holdem
