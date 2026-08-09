#pragma once

#include <array>

#include "antwika/holdem/Card.hpp"
#include "antwika/holdem/Chips.hpp"
#include "antwika/holdem/Limits.hpp"

namespace antwika::holdem
{

    struct Seat final
    {
        Chips stack{};

        Chips committed{};

        Chips roundCommitted{};

        bool occupied = false;

        bool inHand = false;

        bool actedThisRound = false;

        bool mayRaise = true;

        std::array<Card, kHoleCardCount> holeCards{};

        bool operator==(const Seat &other) const = default;
    };

}
