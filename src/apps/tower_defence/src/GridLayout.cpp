#include "antwika/tower_defence/GridLayout.hpp"

#include <antwika/geometry/Grid.hpp>
#include <antwika/gfx/Glyphs.hpp>
#include <antwika/ui/Theme.hpp>

namespace antwika::tower_defence
{

    namespace
    {
        // The same four numbers under two sets of names.
        // GridLayout is what BattleScene and the sink pass around.
        // The arithmetic over it is antwika::geometry's.
        [[nodiscard]] antwika::geometry::Grid gridOf(
            const GridLayout &layout) noexcept
        {
            return antwika::geometry::Grid{
                .origin = layout.origin,
                .cell = layout.cell,
                .columns = layout.width,
                .rows = layout.height};
        }
    } // namespace

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
        const std::uint32_t bar = scoreBarHeight(canvas);
        if (canvas.height <= bar)
        {
            return std::nullopt;
        }

        // What parts this from every other grid in the tree.
        // The strip is taken off the top before anything is fitted.
        // So a click on the bar falls outside the area cells fitted in.
        const auto grid = antwika::geometry::gridFit(
            Rect{
                .origin = Point{.x = 0, .y = static_cast<std::int32_t>(bar)},
                .size = Size{
                    .width = canvas.width, .height = canvas.height - bar}},
            width,
            height);

        if (!grid.has_value())
        {
            return std::nullopt;
        }

        return GridLayout{
            .width = grid->columns,
            .height = grid->rows,
            .cell = grid->cell,
            .origin = grid->origin};
    }

    std::optional<Cell> cellAt(
        const GridLayout &layout,
        const std::int32_t x,
        const std::int32_t y)
    {
        const auto cell = antwika::geometry::cellAt(
            gridOf(layout), Point{.x = x, .y = y});

        if (!cell.has_value())
        {
            return std::nullopt;
        }

        return Cell{.x = cell->column, .y = cell->row};
    }

    Rect cellRect(const GridLayout &layout, const Cell &cell)
    {
        return antwika::geometry::cellRect(
            gridOf(layout),
            antwika::geometry::GridCell{.column = cell.x, .row = cell.y});
    }

} // namespace antwika::tower_defence
