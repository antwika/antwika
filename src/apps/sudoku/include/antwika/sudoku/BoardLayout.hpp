#pragma once

#include <cstdint>
#include <optional>

#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>

#include "antwika/sudoku/Board.hpp"

namespace antwika::sudoku
{

    using antwika::gfx::Point;
    using antwika::gfx::Rect;

    /**
     * @brief Where the 9x9 grid lands inside an area, in pixels.
     *
     * One definition of that geometry, shared by the two things that
     * have to agree on it: SudokuScene, which draws a square at a
     * position, and PlaySink, which decides which square a click at a
     * position fell in.
     * Two copies of this arithmetic would be two chances for the grid
     * somebody sees and the grid they can hit to drift apart, which is
     * the same reason life::BoardLayout exists.
     *
     * The area itself comes off ui::Frame::rects rather than being
     * measured a second time beside the UI, so the grid is placed
     * *from* the layout the bar above it produced.
     */
    struct BoardLayout
    {
        std::uint32_t cell = 0; ///< Pixels along one square's edge.
        Point origin{};         ///< Top-left of the grid.

        /**
         * @brief Compare two layouts.
         * @param other The layout to compare against.
         * @return True when both fields match.
         */
        [[nodiscard]] bool operator==(const BoardLayout &other) const
            = default;
    };

    /**
     * @brief Work out where the grid lands inside an area.
     *
     * Squares are square and a whole number of pixels across, and the
     * grid is centred in whatever is left over. A square is never
     * rounded up to a minimum, so the grid always fits and the centring
     * offsets cannot go negative.
     *
     * @param area The area the grid is drawn into.
     * @return The layout, or nothing for an area too small to give
     * every square a pixel.
     */
    [[nodiscard]] std::optional<BoardLayout> layoutFor(Rect area);

    /**
     * @brief Find which square a point falls in.
     *
     * The coordinates are signed because a pointer is free to report
     * itself outside the area.
     *
     * @param layout Where the grid lies.
     * @param x Horizontal position on the canvas, in pixels.
     * @param y Vertical position on the canvas, in pixels.
     * @return The square containing that point, or nothing when the
     * point lies outside the grid.
     */
    [[nodiscard]] std::optional<Square> cellAt(
        const BoardLayout &layout, std::int32_t x, std::int32_t y);

    /**
     * @brief Get the area one square covers.
     * @param layout Where the grid lies.
     * @param square Which square.
     * @return Its rectangle, in the same pixels the layout is in.
     */
    [[nodiscard]] Rect squareRect(
        const BoardLayout &layout, Square square);

} // namespace antwika::sudoku
