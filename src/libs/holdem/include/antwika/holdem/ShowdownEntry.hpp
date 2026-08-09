#pragma once

#include <array>

#include "antwika/holdem/Card.hpp"
#include "antwika/holdem/HandValue.hpp"
#include "antwika/holdem/Limits.hpp"
#include "antwika/holdem/SeatId.hpp"

namespace antwika::holdem
{

    struct ShowdownEntry final
    {
        SeatId seat{};

        std::array<Card, kHoleCardCount> holeCards{};

        HandValue value{};

        bool operator==(const ShowdownEntry &other) const = default;
    };

}
