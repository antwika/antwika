#pragma once

#include <cstdint>

#include "antwika/game/Cell.hpp"

namespace antwika::game
{

    struct GridExtent final
    {
        std::int32_t width = 0;
        std::int32_t height = 0;

        [[nodiscard]] constexpr bool contains(Cell cell) const noexcept
        {
            return cell.x >= 0 && cell.x < width && cell.y >= 0
                   && cell.y < height;
        }

        [[nodiscard]] bool operator==(
            const GridExtent &other) const = default;
    };

}
