#pragma once

#include "antwika/input/Key.hpp"
#include "antwika/input/KeyModifiers.hpp"
#include "antwika/input/MouseButton.hpp"
#include "antwika/input/Position.hpp"

namespace antwika::input
{

    struct PointerButtonReleased final
    {
        MouseButton button = MouseButton::Left;
        Position position{};
        KeyModifiers modifiers{};

        [[nodiscard]] bool operator==(
            const PointerButtonReleased &other) const = default;
    };

}
