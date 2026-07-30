#pragma once

#include <antwika/gfx/Size.hpp>
#include <antwika/ui/Frame.hpp>
#include <antwika/ui/Pointer.hpp>
#include <antwika/ui/WidgetId.hpp>

#include "antwika/game/Camera.hpp"

namespace antwika::game
{

    using antwika::gfx::Size;
    using antwika::ui::Frame;
    using antwika::ui::Pointer;
    using antwika::ui::WidgetId;

    /**
     * @brief What the toolbar's buttons are called.
     *
     * Symbolic names rather than where a button ended up in the layout,
     * because this is what crosses back into the simulation. None of
     * these ever reaches a replay: what is recorded is the click, and
     * which button it hit is worked out again from it -- see UiSink.
     */
    namespace widgets
    {
        /**
         * @brief Zoom one level out.
         */
        inline constexpr WidgetId kZoomOut{1};

        /**
         * @brief Zoom one level in.
         */
        inline constexpr WidgetId kZoomIn{2};

        /**
         * @brief Put the camera back where the run started.
         */
        inline constexpr WidgetId kResetView{3};
    } // namespace widgets

    /**
     * @brief The bar of buttons drawn over the grid.
     *
     * A pure function of the canvas, the pointer and the camera, so the
     * same three always produce the same picture and the same answer
     * about what the pointer is on.
     *
     * The canvas it is laid out against must be the size the window was
     * *asked* for rather than the size one reports, because a hit-test
     * is a function of the layout and the layout is a function of the
     * canvas: resolving a recorded click against a differently sized
     * window would resolve it to a different button.
     */
    class Toolbar final
    {
    public:
        /**
         * @brief Describe the toolbar for one tick.
         * @param canvas The area the UI is laid out into.
         * @param pointer Where the pointer is and what it is doing.
         * @param camera The camera whose zoom the bar reports.
         * @return The drawing commands and what the pointer did.
         */
        [[nodiscard]] Frame describe(
            Size canvas, Pointer pointer, const Camera &camera) const;
    };

} // namespace antwika::game
