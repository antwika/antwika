#pragma once

#include <array>
#include <cstddef>
#include <vector>

#include "antwika/holdem/Blinds.hpp"
#include "antwika/holdem/Card.hpp"
#include "antwika/holdem/Chips.hpp"
#include "antwika/holdem/Limits.hpp"
#include "antwika/holdem/SeatId.hpp"
#include "antwika/holdem/Stage.hpp"

namespace antwika::holdem
{

    struct TableView final
    {
        SeatId seat{};

        Stage stage{};

        std::array<Card, kHoleCardCount> holeCards{};

        std::vector<Card> board;

        Chips pot{};

        Chips stack{};

        Chips currentBet{};

        Chips toCall{};

        Chips minRaiseTo{};

        Chips maxRaiseTo{};

        bool mayRaise = true;

        std::size_t playersInHand{};

        Blinds blinds{};

        bool operator==(const TableView &other) const = default;
    };

}
