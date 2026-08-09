#pragma once

#include <string>

#include "antwika/gfx/Size.hpp"

namespace antwika::gfx
{

    struct WindowDesc final
    {
        std::string title;

        Size size{.width = 800, .height = 600};

        bool resizable = false;

        bool fullscreen = false;
    };

}
