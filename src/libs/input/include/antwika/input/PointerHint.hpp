#pragma once

#include "antwika/input/Position.hpp"

namespace antwika::input
{

    struct PointerHint final
    {
        Position position{};

        [[nodiscard]] bool operator==(const PointerHint &other) const = default;
    };

}
