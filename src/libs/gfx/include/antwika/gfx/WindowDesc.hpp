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
    };

} // namespace antwika::gfx
