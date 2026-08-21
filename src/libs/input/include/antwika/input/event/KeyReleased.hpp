#pragma once

#include "antwika/input/Key.hpp"
#include "antwika/input/KeyModifiers.hpp"
#include "antwika/input/MouseButton.hpp"
#include "antwika/input/Position.hpp"

namespace antwika::input
{

    struct KeyReleased final
    {
        Key key = Key::A;
        KeyModifiers modifiers{};

        [[nodiscard]] bool operator==(const KeyReleased &other) const = default;
    };

}
