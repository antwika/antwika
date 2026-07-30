#pragma once

#include <antwika/gfx/Size.hpp>

namespace antwika::game
{

    /**
     * @brief The area the toolbar is laid out and hit-tested against.
     *
     * The size the window is *asked* for, which is what UiOverlay is
     * constructed over. It lives here rather than in main.cpp because the
     * app is not its only reader: a test that exercises the shipped wiring
     * has to resolve a click against the same canvas the binary does, and
     * a second literal is a second answer waiting to disagree.
     *
     * Changing this number invalidates every existing recording. Which
     * button a recorded click hits is a function of the layout, and the
     * layout is a function of the canvas, so a click near a button's edge
     * lands on its neighbour -- or on the grid -- under a canvas the
     * recording was not made against. The file still parses and the run
     * still completes; only the outcome differs.
     */
    constexpr gfx::Size kUiCanvas{.width = 1024, .height = 640};

} // namespace antwika::game
