#pragma once

#include <antwika/gfx/IRenderer.hpp>
#include <antwika/gfx/ITexture.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/game/Cell.hpp"
#include "antwika/game/SceneSnapshot.hpp"

namespace antwika::game
{

    using antwika::gfx::IRenderer;
    using antwika::gfx::ITexture;
    using antwika::gfx::Size;

    /**
     * @brief Draws a snapshot: the ground, the roads and the walkers.
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
     * Two things keep the cost proportional to what is on screen rather
     * than to how big the grid is. Only cells whose diamonds reach the
     * canvas are drawn at all, and a cell is one blit whatever it holds.
     */
    class GridScene final
    {
    public:
        /**
         * @brief Draw one frame.
         * @param renderer Receives the drawing calls.
         * @param canvas The size of the area being drawn into.
         * @param snapshot What to draw.
         * @param atlas The texture every tile is blitted from; it must
         * have come from this renderer, and must be the atlas
         * TileAtlas.hpp addresses.
         */
        void draw(
            IRenderer &renderer,
            Size canvas,
            const SceneSnapshot &snapshot,
            const ITexture &atlas) const;

    private:
        void drawGround(
            IRenderer &renderer,
            Size canvas,
            const SceneSnapshot &snapshot,
            const ITexture &atlas) const;

        [[nodiscard]] static bool onCanvas(
            Cell cell, Size canvas, const SceneSnapshot &snapshot);
    };

} // namespace antwika::game
