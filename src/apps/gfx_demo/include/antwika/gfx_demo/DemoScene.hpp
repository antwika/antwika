#pragma once

#include <cstdint>

#include <antwika/gfx/IRenderer.hpp>
#include <antwika/gfx/ITexture.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/ui/DrawList.hpp>
#include <antwika/ui/Frame.hpp>
#include <antwika/ui/Pointer.hpp>
#include <antwika/ui/WidgetId.hpp>

namespace antwika::gfx_demo
{

    using antwika::gfx::IRenderer;
    using antwika::gfx::ITexture;
    using antwika::gfx::Size;
    using antwika::ui::DrawList;
    using antwika::ui::Frame;
    using antwika::ui::Pointer;
    using antwika::ui::WidgetId;

    /**
     * @brief What this demo's buttons are called.
     *
     * Symbolic, and chosen here rather than derived from where a widget
     * ended up in the layout, so the loop compares against a name.
     */
    namespace widgets
    {
        /**
         * @brief The button that counts one more press.
         */
        inline constexpr WidgetId kCount{1};

        /**
         * @brief The button that puts the count back to zero.
         */
        inline constexpr WidgetId kReset{2};
    } // namespace widgets

    /**
     * @brief Draws the demo's picture: a flat background, a row of
     * evenly spaced bars sized relative to the canvas, the logo blitted
     * twice, and a panel of nested layouts laid out through antwika::ui.
     *
     * Stateless and deterministic on purpose. The same canvas size,
     * pointer and count always produce the same drawing calls in the same
     * order, which is what makes the picture assertable against a mock
     * renderer instead of having to be looked at.
     * The texture is a parameter rather than a member for the same
     * reason: a scene that owned one could not be a pure function of
     * what it is handed.
     * The count is a parameter for that reason too: what a button did
     * belongs to whoever runs the frames, not to what draws them.
     */
    class DemoScene final
    {
    public:
        /**
         * @brief Draw one frame.
         * @param renderer Receives the drawing calls.
         * @param canvas The size of the area being drawn into.
         * @param logo Blitted whole above the bars, and left-half only
         * below them, to show a source rectangle and a tint at work.
         * @param overlay The UI picture, painted last so it reads as
         * being in front of the scene.
         */
        void draw(
            IRenderer &renderer,
            Size canvas,
            const ITexture &logo,
            const DrawList &overlay) const;

        /**
         * @brief Build the UI panel's picture without drawing it.
         *
         * Separate from draw() so a test can inspect the whole panel
         * with no renderer involved at all, and so the caller sees what
         * the pointer did before anything is drawn.
         *
         * @param canvas The size of the area being drawn into.
         * @param pointer Where the pointer is and what it is doing.
         * @param clicks How many times the counting button has been
         * pressed so far.
         * @return The drawing commands and what the pointer did to them.
         */
        [[nodiscard]] Frame describe(
            Size canvas,
            Pointer pointer = {},
            std::uint32_t clicks = 0) const;
    };

} // namespace antwika::gfx_demo
