#pragma once

#include <cstdint>

namespace antwika::gfx
{

    struct Color final
    {
        std::uint8_t red = 0;
        std::uint8_t green = 0;
        std::uint8_t blue = 0;
        std::uint8_t alpha = 255;

        [[nodiscard]] bool operator==(const Color &other) const = default;
    };

}
