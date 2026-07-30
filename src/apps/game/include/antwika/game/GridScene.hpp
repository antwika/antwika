#pragma once

#include <antwika/gfx/IRenderer.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/game/Cell.hpp"
#include "antwika/game/SceneSnapshot.hpp"

namespace antwika::game
{

    using antwika::gfx::IRenderer;
    using antwika::gfx::Size;

    /**
     * @brief Draws a snapshot: the ground, the lattice, the paths and the
     * walkers.
     *
     * Stateless and deterministic on purpose, like apps/life's BoardScene
     * and apps/poker's TableScene. The same snapshot and canvas always
     * produce the same drawing calls in the same order, which is what
     * makes the picture assertable against a mock renderer instead of
     * having to be looked at.
     *
     * Three things keep the cost proportional to what is on screen rather
     * than to how big the grid is:
     *
     * The ground is one rectangle. Filled diamonds tessellate the plane,
     * so a ground of one colour needs no per-cell fill and only the tiles
     * that differ from it are drawn.
     *
     * Each cell draws two lattice edges, not four. The other two belong to
     * its neighbours, so every shared edge is drawn once.
     *
     * Only cells whose diamonds reach the canvas are considered at all.
     */
    class GridScene final
    {
    public:
        /**
         * @brief Draw one frame.
         * @param renderer Receives the drawing calls.
         * @param canvas The size of the area being drawn into.
         * @param snapshot What to draw.
         */
        void draw(
            IRenderer &renderer,
            Size canvas,
            const SceneSnapshot &snapshot) const;

    private:
        void drawLattice(
            IRenderer &renderer,
            Size canvas,
            const SceneSnapshot &snapshot) const;

        [[nodiscard]] static bool onCanvas(
            Cell cell, Size canvas, const SceneSnapshot &snapshot);
    };

} // namespace antwika::game
