#pragma once

#include <cstdint>
#include "antwika/geometry/Point.hpp"
#include "antwika/geometry/Rect.hpp"
#include "antwika/geometry/Size.hpp"

namespace antwika::geometry
{

    struct GridCell final
    {
        std::uint32_t column = 0;
        std::uint32_t row = 0;

        [[nodiscard]] bool operator==(const GridCell &other) const
            = default;
    };

}
