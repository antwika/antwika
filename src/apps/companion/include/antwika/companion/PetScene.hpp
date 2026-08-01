#pragma once

#include <cstdint>

#include <antwika/animation/Clip.hpp>
#include <antwika/gfx/IRenderer.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/companion/PetLayout.hpp"
#include "antwika/companion/PetSnapshot.hpp"

namespace antwika::companion
{

    using antwika::gfx::IRenderer;
    using antwika::gfx::Size;

    /**
     * @brief Draws a companion: a sky, some ground, two gauges, a
     * three-line readout and an animal built out of rectangles.
     *
     * **The animal is drawn rather than blitted, and that is a choice
     * rather than a shortcut.** The house style is a hand-drawn PNG
     * atlas addressed arithmetically, which is right when a picture has
     * hundreds of distinct tiles and an artist who is not the
     * programmer. This window is 256 pixels square and the animal is a
     * dozen boxes: an atlas for it would be a checked-in binary, a
     * second contract in TileAtlas.hpp's shape, and a startup check that
     * the file is the size the header says -- all to hold a picture that
     * is shorter written down than described. Drawing it from
     * IRenderer's rectangles also keeps the whole scene assertable
     * against a mock renderer call by call, which a blit of somebody's
     * art is not.
     *
     * Stateless between frames and deterministic, like apps/life's
     * BoardScene: the same snapshot and canvas always produce the same
     * drawing calls in the same order.
     * The clips it owns hold no time of their own either -- they are
     * definitions, resolved against the tick count the snapshot carries,
     * which is what keeps a redrawn frame the same picture on every
     * toolchain and in every replay.
     *
     * Everything is laid out on a square grid of kSceneUnits whole
     * units centred in the canvas, so a canvas that is not square
     * leaves a margin rather than stretching anything, and a canvas too
     * small to give a unit a pixel draws the sky and stops.
     *
     * A speech bubble appears beside the animal whenever the snapshot
     * carries a Saying, and the words it holds are this file's table
     * rather than anything the simulation knows. Which line comes up
     * and how long it stays are `Pet`'s decisions, so the bubble adds
     * no state here: a scene that counted a bubble down would be one
     * holding time a replay has to reproduce.
     *
     * A perished companion is drawn with a "new companion" button over
     * it, which is the one way out of a state nothing else leaves. The
     * scene neither owns it nor decides what pressing it does: where it
     * is is reviveButtonRect()'s answer, shared with the ReviveSink
     * that hit-tests a press against it, and whether to draw it at all
     * is the one thing the snapshot already says.
     *
     * The readout says in words what the gauges say in bars, plus the
     * one thing no bar can: whether the companion is awake, asleep or
     * gone. Every character of it comes off the snapshot, so it reports
     * the run rather than adding to it, and it is anchored to the
     * bottom of the grid so three lines fit whatever a unit is worth.
     */
    class PetScene final
    {
    public:
        /**
         * @brief Build the scene and the idle clips it draws with.
         */
        PetScene();

        /**
         * @brief Draw one frame.
         * @param renderer Receives the drawing calls.
         * @param canvas The size of the area being drawn into.
         * @param snapshot What to draw.
         */
        void draw(
            IRenderer &renderer,
            Size canvas,
            const PetSnapshot &snapshot) const;

    private:
        antwika::animation::Clip breathe;
        antwika::animation::Clip blink;
        antwika::animation::Clip drowse;
    };

} // namespace antwika::companion
