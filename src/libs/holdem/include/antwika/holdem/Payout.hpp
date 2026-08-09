#pragma once

#include "antwika/holdem/Chips.hpp"
#include "antwika/holdem/SeatId.hpp"

namespace antwika::holdem
{

    struct Payout final
    {
        SeatId seat{};

        Chips amount{};

        bool operator==(const Payout &other) const = default;
    };

}
