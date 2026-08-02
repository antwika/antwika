#pragma once

#include <cstdint>

#include <antwika/animation/Clip.hpp>
#include <antwika/gfx/IRenderer.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/companion/Messages.hpp"
#include "antwika/companion/PetLayout.hpp"
#include "antwika/companion/PetSnapshot.hpp"

namespace antwika::companion
{

    using antwika::gfx::IRenderer;
    using antwika::gfx::Size;

    /**
     * @brief Draws a companion: a sky, some ground, four gauges, three
     * props, a three-line readout and an animal built out of
     * rectangles.
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
     * carries a Saying, and which message id that stands for is this
     * file's table rather than anything the simulation knows. Which
     * line comes up and how long it stays are `Pet`'s decisions, so the
     * bubble adds no state here: a scene that counted a bubble down
     * would be one holding time a replay has to reproduce.
     *
     * **Every word it draws comes off an injected i18n::Translator**,
     * and the split that keeps is exactly the one Saying already
     * describes: `Pet` decides *that* something is said and this
     * decides *what*, so no translator ever reaches the simulation and
     * the active language cannot become state a replay reproduces. The
     * bubble is scaled to the longest line the catalogue holds rather
     * than to a character count written here, since a constant taken
     * from the English table would silently be wrong for every other
     * language. Nothing this scene measures is hit-tested -- where the
     * one button is is reviveButtonBox()'s answer and owes nothing to
     * its caption -- so the words may decide pixels and nothing else.
     *
     * A perished companion is drawn with a "new companion" button over
     * it, which is the one way out of a state nothing else leaves. The
     * scene neither owns it nor decides what pressing it does: where it
     * is is reviveButtonRect()'s answer, shared with the ReviveSink
     * that hit-tests a press against it, and whether to draw it at all
     * is the one thing the snapshot already says.
     *
     * The three props are painted into the very boxes `propAt()`
     * hit-tests, so what somebody aims at and what they hit are one set
     * of rectangles. The one the companion would like is lit rather
     * than merely present, which is this application's whole answer to
     * instructions: what to press next is on the screen.
     *
     * **Each of them is named as well as lit**, in the bottom row of
     * its own box: `propArtBox()` is where the picture goes and
     * `propLabelBox()` is where the word does, and the two together are
     * the box a press is tested against. So a label cannot be pressed
     * without pressing the prop it names, and cannot reach into a
     * neighbour's. The words are scaled to the longest label the
     * catalogue in use holds, exactly as the bubble's are, so all three
     * read at one size in every language.
     *
     * The readout says the things no bar can -- whether the companion
     * is awake, asleep or gone, how old it is, what kind of day it is,
     * and the record the file keeps behind it. Every character of it
     * comes off the snapshot, so it reports the run rather than adding
     * to it, and it is anchored to the bottom of the grid so three
     * lines fit whatever a unit is worth.
     */
    class PetScene final
    {
    public:
        /**
         * @brief Build the scene and the idle clips it draws with.
         * @param translator Words everything drawn here. Must outlive
         * this scene.
         */
        explicit PetScene(const Translator &translator);

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
        const Translator &translator;
        antwika::animation::Clip breathe;
        antwika::animation::Clip blink;
        antwika::animation::Clip drowse;
    };

} // namespace antwika::companion
