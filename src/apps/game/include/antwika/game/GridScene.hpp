#pragma once

#include <antwika/animation/Progress.hpp>
#include <antwika/gfx/IRenderer.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/game/AtlasTextures.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Messages.hpp"
#include "antwika/game/SceneSnapshot.hpp"

namespace antwika::game
{

    using antwika::animation::Progress;
    using antwika::gfx::IRenderer;
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
     * Every sprite is one blit from one of the three sheets, addressed
     * through TileAtlas and placed through SpriteBounds. The scene draws
     * no shape of its own: the lattice is painted into the ground
     * sprite's own edges, so a grid line is a property of the art rather
     * than a line the scene has to place, and a junction is a sprite
     * rather than four stubs stepped out by hand.
     *
     * **The terrain and the buildings are painted in one pass, a
     * diagonal of cells at a time.** A sprite overhangs its diamond now
     * -- headroom above it and the base block's skirt below -- so what
     * is in front has to be painted after what it stands before.  A
     * diagonal is exactly the set of cells at one screen depth, so the
     * pass walks x + y upward, lays each diagonal's ground and roads,
     * and then the buildings whose blocks start on it; a cell a building
     * stands on is skipped rather than painted under it, since a
     * building's own art owns its whole footprint.  The walkers still
     * come last, so a walker is never hidden by what it is standing on.
     *
     * Which road sprite a cell shows is decided here, from the
     * snapshot's paths, which arrive in ascending order -- so a
     * neighbour is a binary search rather than a second index the scene
     * would have to be handed and kept in step with.
     *
     * The ghost is drawn last and at reduced alpha, from the same sprite
     * the real placement would use, so a placeholder cannot come to look
     * like something the palette does not place. Where it goes is in the
     * snapshot rather than read off a pointer here: which cell a pixel
     * means is a function of the camera, and the camera is simulation
     * state -- see BuildGhost.
     *
     * It is bordered with the four lines of footprintOutline(), traced
     * round the block's own diamond box, so the outline shows exactly
     * the cells the click would take whatever the art above them does.
     *
     * Two things keep the cost proportional to what is on screen rather
     * than to how big the grid is. Only cells whose sprites reach the
     * canvas are drawn at all, and a cell is at most two blits whatever
     * it holds.
     *
     * **The hover panel is the one thing here drawn as rectangles and
     * text rather than blitted**, since it is not art: it is a box and a
     * few lines that come out of the snapshot as a plain value -- see
     * ReadoutPanel.hpp.
     * It is drawn in a pass of its own, last of all, since it is what
     * somebody is reading.
     *
     * **What a building holds and what a walker carries are read off
     * that panel and nowhere else.** They used to be gauged by small
     * vertical bars floating over every sprite, which put a row of
     * rectangles on top of the art at all times and made a busy district
     * unreadable; asking for a number by pointing at the thing that
     * holds it says the same thing and only when somebody wants it.
     *
     * The panel is drawn here rather than through antwika::ui for one
     * structural reason: this app's UI is described and resolved inside
     * the tick path by UiSink, downstream of the recorder, and what the
     * panel says is worked out from input::PointerHintChannel, which no
     * replay reproduces. A hover panel taken through that path would put
     * an unrecorded value into the tick path, which is exactly what the
     * channel's safety condition forbids.
     */
    class GridScene final
    {
    public:
        /**
         * @brief Construct the scene over the language its hover panel
         * words itself in.
         *
         * The panel is the one thing here made of words rather than of
         * art, so it is the one thing that needs a translator; the
         * sheets say everything else.
         *
         * @param translator Words the hover panel; must outlive this
         * scene.
         */
        explicit GridScene(const Translator &translator);

        /**
         * @brief Draw one frame.
         *
         * The same snapshot drawn at two different sub-tick fractions
         * differs only in where the walkers are, since they are the one
         * thing that moves between two ticks. Everything else is on a
         * cell, and a cell does not move.
         *
         * **A paused snapshot is the same picture at every fraction**,
         * since the sub-tick is ignored while SceneSnapshot::paused says
         * the run is held. That is decided here rather than by whoever
         * supplies the fraction, so a held walker is still wherever it
         * is drawn from.
         *
         * @param renderer Receives the drawing calls.
         * @param canvas The size of the area being drawn into.
         * @param snapshot What to draw.
         * @param atlases The three sheets every sprite is blitted from;
         * they must have come from this renderer, and must be laid out
         * the way TileAtlas.hpp addresses them.
         * @param subTick How far through the tick this frame falls; zero
         * on the frame the tick itself draws.
         */
        void draw(
            IRenderer &renderer,
            Size canvas,
            const SceneSnapshot &snapshot,
            const AtlasTextures &atlases,
            Progress subTick = Progress()) const;

    private:
        // withWalkers rides the walkers along in the terrain pass.
        // Each paints at the deeper of its two cells' screen depths.
        // A building in front of one then hides it; one behind cannot.
        // An open overlay keeps them out and paints them last instead.
        // A walker is a thing in the city, not a fact about it.
        // So over the scrim, where somebody is reading the numbers.
        void drawTerrain(
            IRenderer &renderer,
            Size canvas,
            const SceneSnapshot &snapshot,
            const AtlasTextures &atlases,
            Progress phase,
            bool withWalkers) const;

        void drawWalker(
            IRenderer &renderer,
            Size canvas,
            const WalkerSprite &walker,
            const SceneSnapshot &snapshot,
            const AtlasTextures &atlases,
            Progress phase) const;

        void drawPlan(
            IRenderer &renderer,
            Size canvas,
            const SceneSnapshot &snapshot,
            const AtlasTextures &atlases) const;

        void drawGhost(
            IRenderer &renderer,
            Size canvas,
            const SceneSnapshot &snapshot,
            const AtlasTextures &atlases) const;

        void drawOverlay(
            IRenderer &renderer,
            Size canvas,
            const SceneSnapshot &snapshot,
            const AtlasTextures &atlases) const;

        void drawReadout(
            IRenderer &renderer,
            Size canvas,
            const SceneSnapshot &snapshot) const;

        [[nodiscard]] static bool onCanvas(
            Cell cell, Size canvas, const SceneSnapshot &snapshot);

        const Translator &translator;
    };

} // namespace antwika::game
