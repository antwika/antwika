#pragma once

#include <antwika/gfx/IRenderer.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/ui/DrawList.hpp>
#include <antwika/ui/Frame.hpp>
#include <antwika/ui/Keyboard.hpp>
#include <antwika/ui/Pointer.hpp>

#include "antwika/ui_demo/DemoState.hpp"

namespace antwika::ui_demo
{

    using antwika::gfx::IRenderer;
    using antwika::gfx::Size;
    using antwika::ui::DrawList;
    using antwika::ui::Frame;
    using antwika::ui::Keyboard;
    using antwika::ui::Pointer;

    /**
     * @brief The whole showcase: a picker, the page it names, and a line
     * saying what just happened.
     *
     * Stateless and deterministic, like game::Toolbar and
     * td::describeScoreBar: the same canvas, pointer, keyboard and
     * DemoState always produce the same picture and the same answer
     * about what was pressed -- which is what lets a whole page be
     * asserted with EXPECT_EQ and no window, no mock and no backend.
     *
     * It holds none of the field's characters, the caret, either list's
     * open flag, either selection or the focus.
     * All of those arrive in the state and go back out through the
     * frame, because antwika::ui retains nothing between frames and
     * neither may this.
     *
     * The canvas it is laid out against must be the size the window was
     * *asked* for rather than the size one reports: a hit-test is a
     * function of the layout, and the layout is a function of the
     * canvas, so resolving a recorded click against a differently sized
     * window would resolve it to a different widget.
     */
    class DemoScene final
    {
    public:
        /**
         * @brief Describe the showcase for one tick.
         * @param canvas The area the UI is laid out into.
         * @param pointer Where the pointer is and what it is doing.
         * @param keyboard The key edges and characters this tick.
         * @param state Which page is shown, and everything on it.
         * @return The drawing commands, what the input did, and where
         * every named widget was laid out.
         */
        [[nodiscard]] Frame describe(
            Size canvas,
            Pointer pointer,
            const Keyboard &keyboard,
            const DemoState &state) const;

        /**
         * @brief Draw one frame of the showcase.
         *
         * Clears first, because this application owns the whole window:
         * there is nothing underneath the UI for it to be drawn over.
         *
         * @param renderer Receives the drawing calls.
         * @param picture The commands describe() produced, by way of the
         * overlay DemoSink wrote them into.
         */
        void draw(IRenderer &renderer, const DrawList &picture) const;
    };

} // namespace antwika::ui_demo
