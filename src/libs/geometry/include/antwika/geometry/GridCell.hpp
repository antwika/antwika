#pragma once

#include <array>
#include <cstdint>
#include <optional>

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

    [[nodiscard]] constexpr std::optional<GridCell> getCellWithin(
        const Size gridSize,
        const std::int64_t column,
        const std::int64_t row) noexcept
    {
        if (column < 0 || row < 0
            || column >= static_cast<std::int64_t>(gridSize.width)
            || row >= static_cast<std::int64_t>(gridSize.height))
        {
            return std::nullopt;
        }

        return GridCell{
            .column = static_cast<std::uint32_t>(column),
            .row = static_cast<std::uint32_t>(row)};
    }

}
