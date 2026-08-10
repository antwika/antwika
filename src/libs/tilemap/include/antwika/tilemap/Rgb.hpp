#pragma once

#include <cstdint>

namespace antwika::tilemap
{

    struct Rgb final
    {
        std::uint8_t red = 0;
        std::uint8_t green = 0;
        std::uint8_t blue = 0;

        [[nodiscard]] bool operator==(const Rgb &other) const = default;
    };

}
