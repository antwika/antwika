#pragma once

#include <cstdint>

namespace antwika::input
{

    struct Offset final
    {
        std::int32_t x = 0;
        std::int32_t y = 0;

        [[nodiscard]] bool operator==(const Offset &other) const = default;
    };

}
