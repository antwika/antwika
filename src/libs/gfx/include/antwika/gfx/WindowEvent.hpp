#pragma once

#include <variant>

#include "antwika/gfx/Size.hpp"

namespace antwika::gfx
{

    /**
     * @brief The window's close control was activated.
     *
     * A request, not a fact: the window stays open until something calls
     * IWindow::close(), so an application can ignore it or ask first.
     */
    struct CloseRequested
    {
    };

    /**
     * @brief The window's drawable area changed size.
     */
    struct Resized
    {
        Size size;
    };

    /**
     * @brief Something a window reported since the last poll.
     *
     * Deliberately limited to lifetime events. Keyboard and pointer input
     * belong with the work that feeds live input into replays, which is
     * out of scope until there is a live input source to record from.
     */
    using WindowEvent = std::variant<CloseRequested, Resized>;

} // namespace antwika::gfx
