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

    class FakeDeck final : public IDeck
    {
    public:
        explicit FakeDeck(std::vector<Card> cards)
            : cards(std::move(cards))
        {
        }

        void shuffle() override
        {
            dealt = 0;
            ++shuffles;
        }

        [[nodiscard]] Card deal() override
        {
            if (dealt == cards.size())
            {
                throw DeckExhaustedError("FakeDeck: the script ran out");
            }
            return cards[dealt++];
        }

        [[nodiscard]] std::size_t shuffleCount() const noexcept
        {
            return shuffles;
        }

    private:
        std::vector<Card> cards;
        std::size_t dealt = 0;
        std::size_t shuffles = 0;
    };

}
