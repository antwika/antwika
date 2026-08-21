#pragma once

#include "antwika/input/Key.hpp"
#include "antwika/input/KeyModifiers.hpp"
#include "antwika/input/MouseButton.hpp"
#include "antwika/input/Position.hpp"

namespace antwika::input
{

    struct PointerMoved final
    {
        Position position{};

        [[nodiscard]] bool operator==(
            const PointerMoved &other) const = default;
    };

}
