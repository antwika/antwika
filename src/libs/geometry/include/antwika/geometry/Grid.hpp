#pragma once

#include <cstdint>
#include <optional>

#include "antwika/geometry/Point.hpp"
#include "antwika/geometry/Rect.hpp"
#include "antwika/geometry/Size.hpp"

namespace antwika::geometry
{

    /**
     * @brief One cell's place in a grid, by column and row.
     *
     * It rides with Grid rather than getting a header of its own,
     * because an index means nothing without the grid it indexes.
     */
    struct GridCell
    {
        std::uint32_t column = 0; ///< Cells from the left edge.
        std::uint32_t row = 0;    ///< Cells from the top edge.

        /**
         * @brief Compare two cells.
         * @param other The cell to compare against.
         * @return True when both indices match.
         */
        [[nodiscard]] bool operator==(const GridCell &other) const
            = default;
    };

    /**
     * @brief Where a grid of square cells landed inside an area.
     *
     * Three applications had a copy of this each -- life::BoardLayout,
     * sudoku::BoardLayout and tower_defence::GridLayout -- and each copy
     * carried the same two decisions: a cell is a whole number of pixels
     * so nothing is ever half a pixel wide, and the hit test widens to
     * 64 bits before subtracting the origin so a pointer far outside the
     * area misses rather than wraps.
     * Both are the kind of thing that is right in three places or wrong
     * in one, which is why they are stated here once.
     */
    struct Grid
    {
        Point origin{};            ///< Top-left of the used area.
        std::uint32_t cell = 0;    ///< Pixels along one cell's edge.
        std::uint32_t columns = 0; ///< Cells across.
        std::uint32_t rows = 0;    ///< Cells down.

        /**
         * @brief Compare two grids.
         * @param other The grid to compare against.
         * @return True when every field matches.
         */
        [[nodiscard]] bool operator==(const Grid &other) const = default;
    };

    /**
     * @brief Fit the largest whole-pixel square cell into an area.
     *
     * The cells that fit are centred in whatever is left over.
     * A cell is never rounded up to a minimum size, so the grid always
     * fits inside the area and the centring offsets cannot go negative.
     *
     * @param area The area the grid is placed into.
     * @param columns Cells across; zero is a grid with nothing to draw.
     * @param rows Cells down; zero is a grid with nothing to draw.
     * @return The grid, or nothing when there is nothing to draw: an
     * empty grid, or an area too small to give every cell a pixel.
     */
    [[nodiscard]] constexpr std::optional<Grid> gridFit(
        const Rect area,
        const std::uint32_t columns,
        const std::uint32_t rows) noexcept
    {
        if (columns == 0 || rows == 0)
        {
            return std::nullopt;
        }

        const auto byWidth = area.size.width / columns;
        const auto byHeight = area.size.height / rows;
        const auto cell = byWidth < byHeight ? byWidth : byHeight;

        if (cell == 0)
        {
            return std::nullopt;
        }

        const auto used =
            Size{.width = cell * columns, .height = cell * rows};

        return Grid{
            .origin = {
                .x = area.origin.x
                     + static_cast<std::int32_t>(
                         (area.size.width - used.width) / 2),
                .y = area.origin.y
                     + static_cast<std::int32_t>(
                         (area.size.height - used.height) / 2)},
            .cell = cell,
            .columns = columns,
            .rows = rows};
    }

    /**
     * @brief Find which cell a point falls in.
     *
     * The point is signed because a pointer is free to report itself
     * outside the area, e.g. while a drag continues past the edge of
     * the window.
     *
     * @param grid Where the cells lie.
     * @param at The point to place, in the grid's own pixels.
     * @return The cell holding that point, or nothing when it lies
     * outside the grid -- including for a grid with no cells at all.
     */
    [[nodiscard]] constexpr std::optional<GridCell> cellAt(
        const Grid &grid, const Point at) noexcept
    {
        if (grid.cell == 0)
        {
            return std::nullopt;
        }

        // Widened before subtracting, so the arithmetic cannot overflow.
        // A pointer far outside the area should miss, not wrap.
        const auto localX =
            static_cast<std::int64_t>(at.x) - grid.origin.x;
        const auto localY =
            static_cast<std::int64_t>(at.y) - grid.origin.y;

        if (localX < 0 || localY < 0)
        {
            return std::nullopt;
        }

        const auto column = static_cast<std::uint64_t>(localX) / grid.cell;
        const auto row = static_cast<std::uint64_t>(localY) / grid.cell;

        if (column >= grid.columns || row >= grid.rows)
        {
            return std::nullopt;
        }

        return GridCell{
            .column = static_cast<std::uint32_t>(column),
            .row = static_cast<std::uint32_t>(row)};
    }

    /**
     * @brief Get the pixels one cell occupies.
     * @param grid Where the cells lie.
     * @param cell The cell to place; need not be inside the grid.
     * @return That cell's rectangle, in the grid's own pixels.
     */
    [[nodiscard]] constexpr Rect cellRect(
        const Grid &grid, const GridCell cell) noexcept
    {
        return Rect{
            .origin = {
                .x = grid.origin.x
                     + static_cast<std::int32_t>(cell.column * grid.cell),
                .y = grid.origin.y
                     + static_cast<std::int32_t>(cell.row * grid.cell)},
            .size = {.width = grid.cell, .height = grid.cell}};
    }

} // namespace antwika::geometry
