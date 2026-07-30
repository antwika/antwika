#pragma once

#include <cstdint>

namespace antwika::ui
{

    /**
     * @brief How a button should look.
     *
     * Supplied by the caller, and never worked out here.
     * This library reads no pointer and no keyboard, so it has nothing to
     * work it out from; an application that knows a button is the one in
     * play can still say so, and whatever gains a pointer later has a
     * place to report it without any signature changing.
     */
    enum class ButtonState : std::uint8_t
    {
        Idle = 0,
        Hovered,
        Pressed,
    };

} // namespace antwika::ui
