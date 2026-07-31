#pragma once

#include <antwika/gfx/Size.hpp>
#include <antwika/ui/DrawList.hpp>

namespace antwika::ui_demo
{

    using antwika::gfx::Size;
    using antwika::ui::DrawList;

    /**
     * @brief The showcase's picture, handed from the tick path to the
     * renderer.
     *
     * The one thing two collaborators have to agree on: DemoSink writes
     * it once per tick, RenderSink paints it.
     * A small shared state object rather than one asking the other, so
     * the renderer never has to know what a pointer or a widget is --
     * the same shape as game::UiOverlay and life::DragState.
     *
     * It holds nothing of its own: what goes in is described from the
     * recorded input and this application's own state, so a replay
     * rebuilds exactly the same picture.
     */
    class DemoOverlay final
    {
    public:
        /**
         * @brief Construct the overlay over the size the UI belongs to.
         *
         * The canvas lives here rather than with whoever describes the
         * UI, so nothing can lay the showcase out against one size and
         * hit-test it against another.
         * It is the size the window was *asked* for, never the size a
         * window reports: which widget a recorded click hits must not
         * depend on how a window manager sized a window on the day.
         *
         * @param canvas The area the UI is laid out into.
         */
        explicit DemoOverlay(Size canvas = {});

        /**
         * @brief Get the area the UI is laid out into.
         * @return The canvas this overlay was constructed over.
         */
        [[nodiscard]] Size canvas() const noexcept;

        /**
         * @brief Put this tick's UI in, replacing whatever was there.
         * @param picture The drawing commands, in the order they draw.
         */
        void set(DrawList picture);

        /**
         * @brief Get the UI's picture.
         * @return The drawing commands, empty until the first set().
         */
        [[nodiscard]] const DrawList &commands() const noexcept;

    private:
        Size area;
        DrawList picture;
    };

} // namespace antwika::ui_demo
