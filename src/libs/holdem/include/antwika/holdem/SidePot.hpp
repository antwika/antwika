#pragma once

#include <vector>

#include "antwika/holdem/Chips.hpp"
#include "antwika/holdem/SeatId.hpp"

namespace antwika::holdem
{

    struct SidePot final
    {
        Chips amount{};

        std::vector<SeatId> contenders;

        bool operator==(const SidePot &other) const = default;
    };

}
