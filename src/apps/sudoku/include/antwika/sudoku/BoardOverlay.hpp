#pragma once

#include <antwika/gfx/Size.hpp>
#include <antwika/ui/DrawList.hpp>

namespace antwika::sudoku
{

    using antwika::gfx::Size;
    using antwika::ui::DrawList;

    /**
     * @brief The picture, handed from the tick path to the renderer.
     *
     * The one thing two collaborators have to agree on: PlaySink writes
     * it once per tick, RenderSink paints it.
     * A small shared value rather than one asking the other, so the
     * renderer never has to know what a pointer, a widget or a puzzle
     * is -- the same shape as ui_demo::DemoOverlay and life::DragState.
     *
     * The canvas lives here rather than with whoever describes the
     * picture, so nothing can lay the grid out against one size and
     * hit-test it against another.
     * It is the size the window was *asked* for, never the size a
     * window reports: which square a recorded click hits must not
     * depend on how a window manager sized a window on the day.
     */
    class BoardOverlay final
    {
    public:
        /**
         * @brief Construct the overlay over the size it belongs to.
         * @param canvas The area the picture is laid out into.
         */
        explicit BoardOverlay(Size canvas = {});

        /**
         * @brief Get the area the picture is laid out into.
         * @return The canvas this overlay was constructed over.
         */
        [[nodiscard]] Size canvas() const noexcept;

        /**
         * @brief Put this tick's picture in, replacing what was there.
         * @param picture The drawing commands, in the order they draw.
         */
        void set(DrawList picture);

        /**
         * @brief Get the picture.
         * @return The drawing commands, empty until the first set().
         */
        [[nodiscard]] const DrawList &commands() const noexcept;

    private:
        Size area;
        DrawList picture;
    };

} // namespace antwika::sudoku
