#pragma once

#include <optional>

#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/ui/DrawList.hpp>

#include "antwika/game/BuildTool.hpp"

namespace antwika::game
{

    using antwika::gfx::Size;
    using antwika::ui::DrawList;

    /**
     * @brief The toolbar's picture, whether it is under the pointer, and
     * which tool it has selected.
     *
     * The one thing three collaborators have to agree on: UiSink writes
     * it once per tick, RenderSystem paints it over the grid, and
     * GridSink asks it whether a click was the toolbar's before treating
     * the click as the world's -- and, when it is the world's, which of
     * the palette's tools that click is now for.
     *
     * Two of them write it: UiSink selects when a palette button is
     * pressed, and GridSink puts the palette down when a right press on
     * the grid leaves build mode.
     * Both do it inside the tick path and neither persists an event for
     * it, which is what keeps the selection regenerated rather than
     * recorded -- see GridSink for the rule a right press follows.
     *
     * **The selected tool is simulation state**, in the same sense the
     * camera is: what a recorded click *means* depends on it, so it has
     * to be regenerated rather than recorded. It lives here rather than
     * in the Toolbar because the Toolbar is a pure function described
     * afresh every tick, and it lives here rather than beside the camera
     * because these are exactly the two collaborators that need it.
     *
     * A small shared state object rather than one asking the other, so
     * the renderer need not know what a pointer is and the grid need not
     * know what a button is -- the same shape as life::DragState.
     *
     * It holds nothing of its own: what goes in is described from the
     * recorded input and the simulation state, so a replay rebuilds the
     * same picture and the same answer.
     */
    class UiOverlay final
    {
    public:
        /**
         * @brief Construct the overlay over the size the UI belongs to.
         *
         * The canvas lives here rather than with whoever describes the
         * UI, so nothing can lay the bar out against one size and
         * hit-test it against another. It is the size the window was
         * *asked* for, never the size a window reports: which button a
         * recorded click hits must not depend on how a window manager
         * sized a window on the day.
         *
         * @param canvas The area the UI is laid out into.
         */
        explicit UiOverlay(Size canvas = {});

        /**
         * @brief Get the area the UI is laid out into.
         * @return The canvas this overlay was constructed over.
         */
        [[nodiscard]] Size canvas() const noexcept;

        /**
         * @brief Put this tick's UI in, replacing whatever was there.
         * @param picture The drawing commands, in the order they draw.
         * @param covered Whether the pointer is over anything the UI
         * filled in.
         */
        void set(DrawList picture, bool covered);

        /**
         * @brief Get the UI's picture.
         * @return The drawing commands, empty until the first set().
         */
        [[nodiscard]] const DrawList &commands() const noexcept;

        /**
         * @brief Check whether the UI is under the pointer.
         *
         * **The recorded pointer's answer**, resolved inside the tick
         * path against the frame UiSink described, and therefore the one
         * a sink asks: it is a function of recorded input alone, so a
         * replay arrives at it again.
         *
         * It is deliberately *not* the answer to ask on the render side.
         * Idle motion is thinned out of the recorded stream -- see
         * input::IdleMotionSource -- so this holds where the pointer was
         * at the last press, release or dragged movement, which may be
         * many frames ago. Use coversPoint() with the hint for anything
         * that follows a free-moving pointer.
         *
         * @return True when the recorded pointer is over something the
         * UI drew.
         */
        [[nodiscard]] bool pointerOverUi() const noexcept;

        /**
         * @brief Check whether the UI covers one point of the canvas.
         *
         * **pointerOverUi()'s render-side counterpart, and the whole of
         * why it is a separate question.** A caller drawing something
         * that follows the free-moving pointer -- the build ghost, the
         * hover readout -- has a position no recorded event carries, and
         * asking pointerOverUi() about it gets the answer for a position
         * from some earlier tick. That is what left the ghost hidden
         * from the moment a palette button was pressed until the next
         * press landed somewhere else.
         *
         * This asks the *layout* instead, which is the direction the
         * dependency is allowed to run: the picture is described from
         * recorded input inside the tick path, and a point is measured
         * against it out here. Nothing is written and nothing about the
         * layout is a function of where the free pointer is, so a hint
         * still decides only what is drawn.
         *
         * A filled rectangle is exactly what makes the UI cover a pixel,
         * which is the same test ui::Interactions::pointerOverUi is
         * resolved by -- so the two agree wherever they are handed the
         * same position.
         *
         * @param at The canvas pixel to ask about.
         * @return True when any fill of this tick's picture contains it.
         */
        [[nodiscard]] bool coversPoint(
            antwika::gfx::Point at) const noexcept;

        /**
         * @brief Choose what a left click on the grid now places.
         * @param tool The tool the palette has selected.
         */
        void select(BuildTool tool) noexcept;

        /**
         * @brief Put the palette down, selecting no tool at all.
         *
         * A state of its own rather than a fallback to Road: a left
         * click then places nothing, no ghost is drawn and no palette
         * button is held down, which is what leaving build mode means
         * everywhere else this genre is played.
         * Putting a tool back instead would leave the palette holding
         * one nobody chose, and the grid laying roads for it.
         */
        void clearTool() noexcept;

        /**
         * @brief Get what a left click on the grid places.
         * @return The selected tool, or nullopt once the palette has
         * been put down; Road until something selects another, so a run
         * with no toolbar lays paths as it always did.
         */
        [[nodiscard]] std::optional<BuildTool> tool() const noexcept;

    private:
        Size area;
        DrawList picture;
        bool covered = false;
        std::optional<BuildTool> selected = BuildTool::Road;
    };

} // namespace antwika::game
