#pragma once

#include <cstdint>

namespace antwika::input
{

    struct Position final
    {
        std::int32_t x = 0;
        std::int32_t y = 0;

        [[nodiscard]] bool operator==(const Position &other) const = default;
    };

}
