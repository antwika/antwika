#include "antwika/life/BoardLayout.hpp"

#include <antwika/geometry/Grid.hpp>

namespace antwika::life
{

    namespace
    {
        // The same four numbers under two sets of names.
        // BoardLayout is what BoardScene and the sink pass around.
        // The arithmetic over it is antwika::geometry's.
        [[nodiscard]] antwika::geometry::Grid gridOf(
            const BoardLayout &layout) noexcept
        {
            return antwika::geometry::Grid{
                .origin = layout.origin,
                .cell = layout.cell,
                .columns = layout.width,
                .rows = layout.height};
        }
    } // namespace

    std::optional<BoardLayout> layoutFor(
        Size canvas, std::uint32_t width, std::uint32_t height)
    {
        const auto grid = antwika::geometry::gridFit(
            antwika::geometry::Rect{.origin = Point{}, .size = canvas},
            width,
            height);

        if (!grid.has_value())
        {
            return std::nullopt;
        }

        return BoardLayout{
            .width = grid->columns,
            .height = grid->rows,
            .cell = grid->cell,
            .origin = grid->origin};
    }

    std::optional<CellCoordinate> cellAt(
        const BoardLayout &layout, std::int32_t x, std::int32_t y)
    {
        const auto cell = antwika::geometry::cellAt(
            gridOf(layout), Point{.x = x, .y = y});

        if (!cell.has_value())
        {
            return std::nullopt;
        }

        return CellCoordinate{.x = cell->column, .y = cell->row};
    }

} // namespace antwika::life
