#pragma once

#include <functional>
#include <optional>
#include <vector>

#include "antwika/holdem/Card.hpp"
#include "antwika/holdem/IDeck.hpp"
#include "antwika/holdem/Stage.hpp"

namespace antwika::holdem
{

    class HandFlow final
    {
    public:
        void begin(IDeck &source);

        void end() noexcept;

        void resume(Stage stage, std::vector<Card> board);

        void adopt(IDeck &source) noexcept;

        [[nodiscard]] Stage stage() const noexcept;

        [[nodiscard]] const std::vector<Card> &board() const noexcept;

        [[nodiscard]] bool hasStreetToDeal() const noexcept;

        [[nodiscard]] Card dealCard();

        void dealStreet();

        void toShowdown() noexcept;

    private:
        std::vector<Card> communityCards;
        std::optional<std::reference_wrapper<IDeck>> deck;
        Stage currentStage = Stage::PreFlop;

        [[nodiscard]] IDeck &requireDeck() const;
    };

}
