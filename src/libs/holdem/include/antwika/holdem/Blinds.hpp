#pragma once

#include "antwika/holdem/Chips.hpp"

namespace antwika::holdem
{

    struct Blinds final
    {
        Chips small{};

        Chips big{};

        bool operator==(const Blinds &other) const = default;
    };

}
