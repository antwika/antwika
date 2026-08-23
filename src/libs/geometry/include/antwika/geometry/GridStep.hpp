#pragma once

#include <array>
#include <cstdint>
#include <optional>

#include "antwika/geometry/GridCell.hpp"
#include "antwika/geometry/Size.hpp"

namespace antwika::geometry
{

    struct GridStep final
    {
        std::int64_t acrossStep = 0;
        std::int64_t downStep = 0;

        [[nodiscard]] bool operator==(const GridStep &other) const
            = default;
    };

    inline constexpr std::array<GridStep, 4> kFourNeighbourSteps{
        GridStep{.acrossStep = 1},
        GridStep{.acrossStep = -1},
        GridStep{.downStep = 1},
        GridStep{.downStep = -1}};

    [[nodiscard]] constexpr std::optional<GridCell> steppedFrom(
        const Size gridSize,
        const GridCell fromCell,
        const GridStep step) noexcept
    {
        return getCellWithin(
            gridSize,
            static_cast<std::int64_t>(fromCell.column) + step.acrossStep,
            static_cast<std::int64_t>(fromCell.row) + step.downStep);
    }

}
