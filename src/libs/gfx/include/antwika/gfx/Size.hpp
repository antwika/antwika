#pragma once

#include <cstdint>

namespace antwika::gfx
{

    /**
     * @brief A width and height in pixels.
     */
    struct Size
    {
        std::uint32_t width = 0;
        std::uint32_t height = 0;
    };

} // namespace antwika::gfx
