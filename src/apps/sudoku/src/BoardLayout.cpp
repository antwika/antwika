#include "antwika/sudoku/BoardLayout.hpp"

#include <cstddef>

#include <antwika/geometry/Grid.hpp>

namespace antwika::sudoku
{

    namespace
    {
        constexpr std::uint32_t kSide =
            static_cast<std::uint32_t>(Board::kSize);

        [[nodiscard]] antwika::geometry::Grid gridOf(
            const BoardLayout &layout) noexcept
        {
            return antwika::geometry::Grid{
                .origin = layout.origin,
                .cell = layout.cell,
                .columns = kSide,
                .rows = kSide};
        }
    }

    std::optional<BoardLayout> layoutFor(const Rect area)
    {
        const auto grid = antwika::geometry::gridFit(area, kSide, kSide);

        if (!grid.has_value())
        {
            return std::nullopt;
        }

        return BoardLayout{.cell = grid->cell, .origin = grid->origin};
    }

    std::optional<Square> cellAt(
        const BoardLayout &layout,
        const std::int32_t x,
        const std::int32_t y)
    {
        const auto cell = antwika::geometry::cellAt(
            gridOf(layout), Point{.x = x, .y = y});

        if (!cell.has_value())
        {
            return std::nullopt;
        }

        return Square{
            .row = static_cast<std::size_t>(cell->row),
            .col = static_cast<std::size_t>(cell->column)};
    }

    Rect squareRect(const BoardLayout &layout, const Square square)
    {
        return antwika::geometry::cellRect(
            gridOf(layout),
            antwika::geometry::GridCell{
                .column = static_cast<std::uint32_t>(square.col),
                .row = static_cast<std::uint32_t>(square.row)});
    }

}
