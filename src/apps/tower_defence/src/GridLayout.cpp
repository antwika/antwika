#include "antwika/tower_defence/GridLayout.hpp"

#include <algorithm>

#include <antwika/gfx/Glyphs.hpp>
#include <antwika/ui/Theme.hpp>

namespace antwika::tower_defence
{

    std::uint32_t scoreBarHeight(const Size canvas) noexcept
    {
        // A panel's padding above and below one line of text.
        // Exactly what describeScoreBar() asks antwika::ui to lay out.
        constexpr std::uint32_t kUnscaled =
            2 * antwika::ui::Theme{}.padding
            + antwika::gfx::kGlyphLineHeight;

        return kUnscaled * antwika::ui::scaleForCanvas(canvas);
    }

    std::optional<GridLayout> layoutFor(
        const Size canvas,
        const std::uint32_t width,
        const std::uint32_t height)
    {
        if (width == 0 || height == 0)
        {
            return std::nullopt;
        }

        const std::uint32_t bar = scoreBarHeight(canvas);
        if (canvas.height <= bar)
        {
            return std::nullopt;
        }

        const std::uint32_t usable = canvas.height - bar;
        const std::uint32_t cell =
            std::min(canvas.width / width, usable / height);
        if (cell == 0)
        {
            return std::nullopt;
        }

        const std::uint32_t drawnWidth = cell * width;
        const std::uint32_t drawnHeight = cell * height;
        return GridLayout{
            .width = width,
            .height = height,
            .cell = cell,
            .origin = Point{
                .x = static_cast<std::int32_t>(
                    (canvas.width - drawnWidth) / 2),
                .y = static_cast<std::int32_t>(
                    bar + ((usable - drawnHeight) / 2))}};
    }

    std::optional<Cell> cellAt(
        const GridLayout &layout,
        const std::int32_t x,
        const std::int32_t y)
    {
        const std::int64_t localX =
            static_cast<std::int64_t>(x) - layout.origin.x;
        const std::int64_t localY =
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

        return Cell{
            .x = static_cast<std::uint32_t>(column),
            .y = static_cast<std::uint32_t>(row)};
    }

    Rect cellRect(const GridLayout &layout, const Cell &cell)
    {
        return Rect{
            .origin = Point{
                .x = layout.origin.x
                    + static_cast<std::int32_t>(cell.x * layout.cell),
                .y = layout.origin.y
                    + static_cast<std::int32_t>(cell.y * layout.cell)},
            .size = Size{.width = layout.cell, .height = layout.cell}};
    }

} // namespace antwika::tower_defence
