#pragma once

#include "antwika/input/Key.hpp"
#include "antwika/input/KeyModifiers.hpp"
#include "antwika/input/MouseButton.hpp"
#include "antwika/input/Position.hpp"

namespace antwika::input
{

    struct KeyPressed final
    {
        Key key = Key::A;
        KeyModifiers modifiers{};

        bool repeat = false;

        [[nodiscard]] bool operator==(const KeyPressed &other) const = default;
    };

}
