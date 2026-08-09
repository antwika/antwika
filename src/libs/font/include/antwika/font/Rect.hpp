#pragma once

#include <cstdint>

namespace antwika::font
{

    struct Rect final
    {
        std::uint32_t x = 0;
        std::uint32_t y = 0;
        std::uint32_t width = 0;
        std::uint32_t height = 0;

        [[nodiscard]] bool operator==(const Rect &other) const = default;
    };

}
