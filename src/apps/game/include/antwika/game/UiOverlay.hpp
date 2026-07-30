#pragma once

#include <antwika/gfx/Size.hpp>
#include <antwika/ui/DrawList.hpp>

namespace antwika::game
{

    using antwika::gfx::Size;
    using antwika::ui::DrawList;

    /**
     * @brief The toolbar's picture, and whether it is under the pointer.
     *
     * The one thing three collaborators have to agree on: UiSink writes
     * it once per tick, RenderSystem paints it over the grid, and
     * GridSink asks it whether a click was the toolbar's before treating
     * the click as the world's.
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
         * @return True when the pointer is over something the UI drew.
         */
        [[nodiscard]] bool pointerOverUi() const noexcept;

    private:
        Size area;
        DrawList picture;
        bool covered = false;
    };

} // namespace antwika::game
