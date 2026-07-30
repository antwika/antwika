#pragma once

#include <antwika/gfx/IRenderer.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/life/Board.hpp"

namespace antwika::life
{

    using antwika::gfx::IRenderer;
    using antwika::gfx::Size;

    /**
     * @brief Draws a Board: a flat background, the board's area, and one
     * filled square per living cell.
     *
     * Stateless and deterministic on purpose, like apps/gfx_demo's
     * DemoScene. The same board and canvas always produce the same drawing
     * calls in the same order, which is what makes the picture assertable
     * against a mock renderer instead of having to be looked at.
     *
     * Cells are square and the board is centred, so a canvas whose aspect
     * ratio does not match the board's leaves a margin rather than
     * stretching anything.
     */
    class BoardScene final
    {
    public:
        /**
         * @brief Draw one frame.
         * @param renderer Receives the drawing calls.
         * @param canvas The size of the area being drawn into.
         * @param board The cell states to draw.
         */
        void draw(
            IRenderer &renderer, Size canvas, const Board &board) const;
    };

} // namespace antwika::life
