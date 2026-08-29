#pragma once

#include <cstdint>
#include <string>

#include "antwika/gfx/Size.hpp"

namespace antwika::gfx
{

    struct WindowSpec final
    {
        std::string title;

        Size size{.width = 800, .height = 600};

        bool resizable = false;

        bool fullscreen = false;

        bool hidden = false;

        std::uint32_t targetFps = 0;
    };

}
