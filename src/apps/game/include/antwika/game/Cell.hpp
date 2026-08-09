#pragma once

#include <compare>
#include <cstdint>

namespace antwika::game
{

    struct Cell final
    {
        std::int32_t x = 0;
        std::int32_t y = 0;

        [[nodiscard]] bool operator==(const Cell &other) const = default;

        [[nodiscard]] std::strong_ordering operator<=>(
            const Cell &other) const = default;
    };

}
