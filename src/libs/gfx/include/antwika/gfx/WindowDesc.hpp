#pragma once

#include <string>

#include "antwika/gfx/Size.hpp"

namespace antwika::gfx
{

    /**
     * @brief What a window should look like when it is created.
     */
    struct WindowDesc
    {
        std::string title;

        /**
         * @brief The size to ask the window system for.
         *
         * This is the *configured* size, and IWindow::configuredSize()
         * reports it back unchanged for as long as the window lives.
         * It is the only window size an application may lay out or
         * hit-test against; see resizable for why.
         */
        Size size{.width = 800, .height = 600};

        /**
         * @brief Whether the user may resize the window.
         *
         * Off by default, which is what keeps every existing window
         * meaning exactly what it did before this field existed, and is
         * also what makes Resized events rare: a window nobody can drag
         * the edge of only ever changes size if the window manager says
         * so.
         *
         * Turning it on changes what IWindow::size() may report, and
         * nothing else. A resize is not simulation input: it does not
         * reach the tick loop, it is not recorded, and a replay of a
         * recorded session must land in the same place whatever size the
         * window happens to be. So an application that turns this on
         * must still lay out and hit-test against configuredSize(), and
         * may use the reported size only to place what it draws inside
         * the drawable area -- see docs/resizable-windows.md.
         */
        bool resizable = false;
    };

} // namespace antwika::gfx
