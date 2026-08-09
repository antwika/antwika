#pragma once

#include <cstdint>

namespace antwika::geometry
{

    struct Point final
    {
        std::int32_t x = 0;
        std::int32_t y = 0;

        [[nodiscard]] bool operator==(const Point &other) const = default;
    };

}
