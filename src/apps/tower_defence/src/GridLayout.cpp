#include "antwika/tower_defence/GridLayout.hpp"

#include <antwika/geometry/Grid.hpp>
#include <antwika/gfx/Glyphs.hpp>
#include <antwika/ui/Theme.hpp>

namespace antwika::tower_defence
{

    namespace
    {
        [[nodiscard]] antwika::geometry::GridCell cellOf(
            const Cell &cell) noexcept
        {
            return antwika::geometry::GridCell{
                .column = cell.x, .row = cell.y};
        }
    }

    std::uint32_t scoreBarHeight(const Size canvas) noexcept
    {
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
        return antwika::geometry::gridFitBelow(
            canvas, scoreBarHeight(canvas), width, height);
    }

    std::optional<Cell> cellAt(
        const GridLayout &layout,
        const std::int32_t x,
        const std::int32_t y)
    {
        const auto cell = antwika::geometry::cellAt(
            layout, antwika::geometry::Point{.x = x, .y = y});

        if (!cell.has_value())
        {
            return std::nullopt;
        }

        return Cell{.x = cell->column, .y = cell->row};
    }

    Rect cellRect(const GridLayout &layout, const Cell &cell)
    {
        return antwika::geometry::cellRect(layout, cellOf(cell));
    }

}
