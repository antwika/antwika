#pragma once

#include <cstdint>

namespace antwika::atlas_editor
{

    struct Pixel final
    {
        std::int32_t x = 0;
        std::int32_t y = 0;

        [[nodiscard]] bool operator==(const Pixel &other) const = default;
    };

}
