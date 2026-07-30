#pragma once

#include <cstddef>
#include <utility>
#include <vector>

#include <antwika/holdem/Card.hpp>
#include <antwika/holdem/DeckExhaustedError.hpp>
#include <antwika/holdem/IDeck.hpp>

namespace antwika::holdem::fakes
{

    using antwika::holdem::Card;
    using antwika::holdem::IDeck;

    /**
     * @brief IDeck that deals a scripted card order.
     *
     * Lets a test name the exact board and hole cards it wants instead
     * of hunting for a seed that happens to produce them.
     */
    class FakeDeck final : public IDeck
    {
    public:
        /**
         * @brief Construct the deck over the cards it should deal.
         * @param cards The cards, in the order deal() will hand them
         * out.
         */
        explicit FakeDeck(std::vector<Card> cards)
            : cards(std::move(cards))
        {
        }

        /**
         * @brief Rewind to the start of the scripted order.
         */
        void shuffle() override
        {
            dealt = 0;
            ++shuffles;
        }

        /**
         * @brief Take the next scripted card.
         * @return That card.
         * @throws DeckExhaustedError If the script ran out.
         */
        [[nodiscard]] Card deal() override
        {
            if (dealt == cards.size())
            {
                throw DeckExhaustedError("FakeDeck: the script ran out");
            }
            return cards[dealt++];
        }

        /**
         * @brief Count how many times the deck was shuffled.
         * @return The shuffle count.
         */
        [[nodiscard]] std::size_t shuffleCount() const noexcept
        {
            return shuffles;
        }

    private:
        std::vector<Card> cards;
        std::size_t dealt = 0;
        std::size_t shuffles = 0;
    };

} // namespace antwika::holdem::fakes
