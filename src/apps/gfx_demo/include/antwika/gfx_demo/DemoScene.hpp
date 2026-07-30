#pragma once

#include <antwika/gfx/IRenderer.hpp>
#include <antwika/gfx/ITexture.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/ui/DrawList.hpp>

namespace antwika::gfx_demo
{

    using antwika::gfx::IRenderer;
    using antwika::gfx::ITexture;
    using antwika::gfx::Size;
    using antwika::ui::DrawList;

    /**
     * @brief Draws the demo's picture: a flat background, a row of
     * evenly spaced bars sized relative to the canvas, the logo blitted
     * twice, and a panel of nested layouts laid out through antwika::ui.
     *
     * Stateless and deterministic on purpose. The same canvas size always
     * produces the same drawing calls in the same order, which is what
     * makes the picture assertable against a mock renderer instead of
     * having to be looked at.
     * The texture is a parameter rather than a member for the same
     * reason: a scene that owned one could not be a pure function of
     * what it is handed.
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
         */
        void draw(
            IRenderer &renderer, Size canvas, const ITexture &logo) const;

        /**
         * @brief Build the UI panel's picture without drawing it.
         *
         * Separate from draw() so a test can inspect the whole panel
         * with no renderer involved at all.
         *
         * @param canvas The size of the area being drawn into.
         * @return The drawing commands, in the order they are drawn.
         */
        [[nodiscard]] DrawList describe(Size canvas) const;
    };

} // namespace antwika::gfx_demo
