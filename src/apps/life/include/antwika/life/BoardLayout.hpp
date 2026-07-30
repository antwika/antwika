#pragma once

#include <cstdint>
#include <optional>

#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Size.hpp>

namespace antwika::life
{

    using antwika::gfx::Point;
    using antwika::gfx::Size;

    /**
     * @brief Where a board of a given size lands on a canvas, in pixels.
     *
     * One definition of that geometry, shared by the two things that must
     * agree on it: BoardScene, which draws a cell at a position, and
     * PointerToggleSink, which decides which cell a click at a position
     * fell in. Two copies of this arithmetic would be two chances for the
     * board someone sees and the board they can hit to drift apart.
     */
    struct BoardLayout
    {
        std::uint32_t width = 0;  ///< Columns on the board.
        std::uint32_t height = 0; ///< Rows on the board.
        std::uint32_t cell = 0;   ///< Pixels along one cell's edge.
        Point origin{};           ///< Top-left of the board on canvas.

        /**
         * @brief Compare two layouts.
         * @param other The layout to compare against.
         * @return True when every field matches.
         */
        [[nodiscard]] bool operator==(const BoardLayout &other) const
            = default;
    };

    /**
     * @brief One cell's place on the board.
     */
    struct CellCoordinate
    {
        std::uint32_t x = 0;
        std::uint32_t y = 0;

        /**
         * @brief Compare two coordinates.
         * @param other The coordinate to compare against.
         * @return True when both coordinates match.
         */
        [[nodiscard]] bool operator==(const CellCoordinate &other) const
            = default;
    };

    /**
     * @brief Work out where a board of this size lands on this canvas.
     *
     * Cells are square and a whole number of pixels across, and the board
     * is centred in whatever space is left over. Cells are never rounded
     * up to a minimum size, so the board always fits inside the canvas
     * and the centring offsets cannot go negative.
     *
     * @param canvas Size of the surface the board is drawn on.
     * @param width Number of columns on the board.
     * @param height Number of rows on the board.
     * @return The layout, or nullopt when there is nothing to draw: an
     * empty board, or a canvas too small to give every cell a pixel.
     */
    [[nodiscard]] std::optional<BoardLayout> layoutFor(
        Size canvas, std::uint32_t width, std::uint32_t height);

    /**
     * @brief Find which cell a point on the canvas falls in.
     *
     * The coordinates are signed because a pointer is free to report
     * itself outside the surface, e.g. while a drag continues past the
     * edge of the window.
     *
     * @param layout Where the board lies on the canvas.
     * @param x Horizontal position on the canvas, in pixels.
     * @param y Vertical position on the canvas, in pixels.
     * @return The cell containing that point, or nullopt when the point
     * lies outside the board -- including for a layout with no cells.
     */
    [[nodiscard]] std::optional<CellCoordinate> cellAt(
        const BoardLayout &layout, std::int32_t x, std::int32_t y);

} // namespace antwika::life
