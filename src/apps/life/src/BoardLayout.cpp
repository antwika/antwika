#include "antwika/life/BoardLayout.hpp"

namespace antwika::life
{

    std::optional<BoardLayout> layoutFor(
        Size canvas, std::uint32_t width, std::uint32_t height)
    {
        if (width == 0 || height == 0)
        {
            return std::nullopt;
        }

        const auto byWidth = canvas.width / width;
        const auto byHeight = canvas.height / height;
        const auto cell = byWidth < byHeight ? byWidth : byHeight;

        if (cell == 0)
        {
            return std::nullopt;
        }

        const auto used =
            Size{.width = cell * width, .height = cell * height};

        return BoardLayout{
            .width = width,
            .height = height,
            .cell = cell,
            .origin = {
                .x = static_cast<std::int32_t>(
                    (canvas.width - used.width) / 2),
                .y = static_cast<std::int32_t>(
                    (canvas.height - used.height) / 2)}};
    }

    std::optional<CellCoordinate> cellAt(
        const BoardLayout &layout, std::int32_t x, std::int32_t y)
    {
        if (layout.cell == 0)
        {
            return std::nullopt;
        }

        // Widened before subtracting, so the arithmetic cannot overflow.
        // A pointer far outside the surface should miss, not wrap.
        const auto localX =
            static_cast<std::int64_t>(x) - layout.origin.x;
        const auto localY =
            static_cast<std::int64_t>(y) - layout.origin.y;

        if (localX < 0 || localY < 0)
        {
            return std::nullopt;
        }

        const auto column =
            static_cast<std::uint64_t>(localX) / layout.cell;
        const auto row = static_cast<std::uint64_t>(localY) / layout.cell;

        if (column >= layout.width || row >= layout.height)
        {
            return std::nullopt;
        }

        return CellCoordinate{
            .x = static_cast<std::uint32_t>(column),
            .y = static_cast<std::uint32_t>(row)};
    }

} // namespace antwika::life
