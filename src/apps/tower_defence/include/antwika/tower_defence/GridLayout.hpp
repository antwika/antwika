#pragma once

#include <cstdint>
#include <optional>

#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/tower_defence/Level.hpp"

namespace antwika::tower_defence
{

    using antwika::gfx::Point;
    using antwika::gfx::Rect;
    using antwika::gfx::Size;

    /**
     * @brief Pixels reserved along the top of the canvas for the score.
     *
     * The grid is laid out below it rather than under it.
     * That is what makes a click on the score bar hit no cell at all,
     * with no sink having to ask a UI whether it covered the pointer.
     */
    inline constexpr std::uint32_t kScoreBarHeight = 48;

    /**
     * @brief Where the grid lands on the canvas, in pixels.
     *
     * One definition of that geometry, shared by the two things that
     * must agree on it: BattleScene, which draws a cell at a position,
     * and TowerPlacementSink, which decides which cell a click fell in.
     * Two copies of this arithmetic would be two chances for the board
     * somebody sees and the board they can build on to drift apart.
     */
    struct GridLayout
    {
        std::uint32_t width = 0;  ///< Columns in the grid.
        std::uint32_t height = 0; ///< Rows in the grid.
        std::uint32_t cell = 0;   ///< Pixels along one cell's edge.
        Point origin{};           ///< Top-left of the grid on canvas.

        [[nodiscard]] bool operator==(const GridLayout &other) const
            = default;
    };

    /**
     * @brief Work out where a grid of this size lands on this canvas.
     *
     * Cells are square and a whole number of pixels across, and the grid
     * is centred in whatever is left below the score bar.
     * Nothing is ever rounded up, so the grid always fits and the
     * centring offsets cannot go negative.
     *
     * @param canvas Size of the surface being drawn into.
     * @param width Columns in the grid.
     * @param height Rows in the grid.
     * @return The layout, or nullopt when there is nothing to draw: an
     * empty grid, or a canvas too small to give every cell a pixel.
     */
    [[nodiscard]] std::optional<GridLayout> layoutFor(
        Size canvas, std::uint32_t width, std::uint32_t height);

    /**
     * @brief Find which cell a point on the canvas falls in.
     *
     * The coordinates are signed because a pointer is free to report
     * itself outside the surface.
     *
     * @param layout Where the grid lies on the canvas.
     * @param x Horizontal position on the canvas, in pixels.
     * @param y Vertical position on the canvas, in pixels.
     * @return The cell containing that point, or nullopt when the point
     * lies outside the grid -- the score bar included.
     */
    [[nodiscard]] std::optional<Cell> cellAt(
        const GridLayout &layout, std::int32_t x, std::int32_t y);

    /**
     * @brief Get the pixels one cell occupies.
     * @param layout Where the grid lies on the canvas.
     * @param cell The cell to place; need not be inside the grid.
     * @return That cell's rectangle on the canvas.
     */
    [[nodiscard]] Rect cellRect(const GridLayout &layout, const Cell &cell);

} // namespace antwika::tower_defence
