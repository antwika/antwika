#pragma once

#include <antwika/gfx/IRenderer.hpp>
#include <antwika/gfx/ITexture.hpp>
#include <antwika/gfx/Size.hpp>

namespace antwika::gfx_demo
{

    using antwika::gfx::IRenderer;
    using antwika::gfx::ITexture;
    using antwika::gfx::Size;

    /**
     * @brief Draws the demo's picture: a flat background, a row of
     * evenly spaced bars sized relative to the canvas, and the logo
     * blitted twice.
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
    };

} // namespace antwika::gfx_demo
