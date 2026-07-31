#include "antwika/holdem/Deck.hpp"

#include <cstddef>
#include <utility>

#include "antwika/holdem/Card.hpp"
#include "antwika/holdem/DeckExhaustedError.hpp"

namespace antwika::holdem
{

    Deck::Deck(rng::IRng &rng) : rng(rng)
    {
        for (std::size_t index = 0; index < kCardCount; ++index)
        {
            cards[index] = static_cast<Card>(index);
        }
    }

    void Deck::shuffle()
    {
        dealt = 0;
        for (std::size_t index = kCardCount - 1; index > 0; --index)
        {
            const auto pick =
                static_cast<std::size_t>(rng.next() % (index + 1));
            std::swap(cards[index], cards[pick]);
        }
    }

    Card Deck::deal()
    {
        if (dealt == kCardCount)
        {
            throw DeckExhaustedError("Deck: no cards left to deal");
        }
        return cards[dealt++];
    }

    std::size_t Deck::remaining() const noexcept
    {
        return kCardCount - dealt;
    }

} // namespace antwika::holdem
