#pragma once

#include <cstdint>

namespace antwika::life
{

    struct CellCoordinate final
    {
        std::uint32_t x = 0;
        std::uint32_t y = 0;

        [[nodiscard]] bool operator==(const CellCoordinate &other) const
            = default;
    };

}
