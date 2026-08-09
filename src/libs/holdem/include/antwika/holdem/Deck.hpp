#pragma once

#include <array>
#include <cstddef>

#include <antwika/rng/IRng.hpp>

#include "antwika/holdem/Card.hpp"
#include "antwika/holdem/IDeck.hpp"
#include "antwika/holdem/TableMemory.hpp"

namespace antwika::holdem
{

    class Deck final : public IDeck
    {
    public:
        explicit Deck(rng::IRng &rng);

        void shuffle() override;

        [[nodiscard]] Card deal() override;

        [[nodiscard]] std::size_t remaining() const noexcept;

        [[nodiscard]] DeckMemory remember() const;

        void restore(const DeckMemory &memory);

    private:
        rng::IRng &rng;
        std::array<Card, kCardCount> cards{};
        std::size_t dealt = 0;
    };

}
