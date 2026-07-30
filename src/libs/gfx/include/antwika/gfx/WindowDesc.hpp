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
        Size size{.width = 800, .height = 600};

        /**
         * @brief Whether the user may resize the window.
         *
         * Off by default, which is also what makes Resized events rare:
         * a window nobody can drag the edge of only ever changes size if
         * the window manager says so.
         */
        bool resizable = false;
    };

} // namespace antwika::gfx
