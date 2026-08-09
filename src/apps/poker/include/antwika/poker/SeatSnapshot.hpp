#pragma once

#include <array>
#include <string>

#include <antwika/holdem/Card.hpp>
#include <antwika/holdem/Chips.hpp>
#include <antwika/holdem/Limits.hpp>

namespace antwika::poker
{

    using antwika::holdem::Card;
    using antwika::holdem::Chips;
    using antwika::holdem::kHoleCardCount;

    struct SeatSnapshot final
    {
        std::string name{};

        Chips stack{};

        Chips committed{};

        Chips roundCommitted{};

        std::array<Card, kHoleCardCount> holeCards{};

        bool occupied = false;

        bool inHand = false;

        bool isButton = false;

        bool isToAct = false;

        bool operator==(const SeatSnapshot &other) const = default;
    };

}
