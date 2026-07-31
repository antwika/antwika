#pragma once

#include <antwika/animation/Progress.hpp>
#include <antwika/gfx/IRenderer.hpp>
#include <antwika/gfx/ITexture.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/game/Cell.hpp"
#include "antwika/game/SceneSnapshot.hpp"

namespace antwika::game
{

    using antwika::animation::Progress;
    using antwika::gfx::IRenderer;
    using antwika::gfx::ITexture;
    using antwika::gfx::Size;

    /**
     * @brief Draws a snapshot: the ground, the roads, the buildings, the
     * walkers and the placement ghost.
     *
     * Stateless and deterministic on purpose, like apps/life's BoardScene
     * and apps/poker's TableScene. The same snapshot and canvas always
     * produce the same drawing calls in the same order, which is what
     * makes the picture assertable against a mock renderer instead of
     * having to be looked at.
     *
     * Every tile is one blit from one atlas texture, addressed through
     * TileAtlas. The scene draws no shape of its own: the lattice is
     * painted into the ground tile's own edges, so a grid line is a
     * property of the art rather than a line the scene has to place, and
     * a junction is a tile rather than four stubs stepped out by hand.
     *
     * Which road tile a cell shows is decided here, from the snapshot's
     * paths, which arrive in ascending order -- so a neighbour is a
     * binary search rather than a second index the scene would have to be
     * handed and kept in step with.
     *
     * The ghost is drawn last and at reduced alpha, from the same tile
     * the real placement would use, so a placeholder cannot come to look
     * like something the palette does not place. Where it goes is in the
     * snapshot rather than read off a pointer here: which cell a pixel
     * means is a function of the camera, and the camera is simulation
     * state -- see BuildGhost.
     *
     * Two things keep the cost proportional to what is on screen rather
     * than to how big the grid is. Only cells whose diamonds reach the
     * canvas are drawn at all, and a cell is one blit whatever it holds.
     */
    class GridScene final
    {
    public:
        /**
         * @brief Draw one frame.
         *
         * The same snapshot drawn at two different sub-tick fractions
         * differs only in where the walkers are, since they are the one
         * thing that moves between two ticks. Everything else is on a
         * cell, and a cell does not move.
         *
         * @param renderer Receives the drawing calls.
         * @param canvas The size of the area being drawn into.
         * @param snapshot What to draw.
         * @param atlas The texture every tile is blitted from; it must
         * have come from this renderer, and must be laid out the way
         * TileAtlas.hpp addresses it.
         * @param subTick How far through the tick this frame falls; zero
         * on the frame the tick itself draws.
         */
        void draw(
            IRenderer &renderer,
            Size canvas,
            const SceneSnapshot &snapshot,
            const ITexture &atlas,
            Progress subTick = Progress()) const;

    private:
        void drawGround(
            IRenderer &renderer,
            Size canvas,
            const SceneSnapshot &snapshot,
            const ITexture &atlas) const;

        void drawGhost(
            IRenderer &renderer,
            Size canvas,
            const SceneSnapshot &snapshot,
            const ITexture &atlas) const;

        [[nodiscard]] static bool onCanvas(
            Cell cell, Size canvas, const SceneSnapshot &snapshot);
    };

} // namespace antwika::game
