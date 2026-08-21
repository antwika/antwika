#pragma once

#include <cstdint>

namespace antwika::ui
{

    enum class ButtonState : std::uint8_t
    {
        Idle = 0,
        Hovered,
        Pressed,
    };

}
