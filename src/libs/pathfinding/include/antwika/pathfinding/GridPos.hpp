#pragma once

#include <compare>
#include <cstdint>

namespace antwika::pathfinding
{

    struct GridPos final
    {
        std::int32_t x = 0;

        std::int32_t y = 0;

        std::int32_t z = 0;

        [[nodiscard]] bool operator==(const GridPos &other) const
            = default;

        [[nodiscard]] std::strong_ordering operator<=>(
            const GridPos &other) const
            = default;
    };

}
