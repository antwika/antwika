#include "antwika/holdem/HandFlow.hpp"

#include <cstddef>
#include <cstdint>

#include "antwika/holdem/Card.hpp"
#include "antwika/holdem/IDeck.hpp"
#include "antwika/holdem/Limits.hpp"
#include "antwika/holdem/Stage.hpp"
#include "antwika/holdem/TableStateError.hpp"

namespace antwika::holdem
{

    void HandFlow::begin(IDeck &source)
    {
        deck = source;
        source.shuffle();
        communityCards.clear();
        currentStage = Stage::PreFlop;
    }

    void HandFlow::end() noexcept
    {
        deck.reset();
    }

    Stage HandFlow::stage() const noexcept
    {
        return currentStage;
    }

    const std::vector<Card> &HandFlow::board() const noexcept
    {
        return communityCards;
    }

    bool HandFlow::hasStreetToDeal() const noexcept
    {
        return currentStage < Stage::River;
    }

    Card HandFlow::dealCard()
    {
        return requireDeck().deal();
    }

    void HandFlow::dealStreet()
    {
        if (!hasStreetToDeal())
        {
            throw TableStateError("HandFlow: the whole board is already out");
        }

        auto &source = requireDeck();
        currentStage = static_cast<Stage>(
            static_cast<std::uint8_t>(currentStage) + 1);

        const auto count =
            currentStage == Stage::Flop ? kFlopSize : std::size_t{1};
        for (std::size_t index = 0; index < count; ++index)
        {
            communityCards.push_back(source.deal());
        }
    }

    void HandFlow::toShowdown() noexcept
    {
        currentStage = Stage::Showdown;
    }

    IDeck &HandFlow::requireDeck() const
    {
        if (!deck)
        {
            throw TableStateError("HandFlow: no hand has been begun");
        }
        return deck->get();
    }

} // namespace antwika::holdem
