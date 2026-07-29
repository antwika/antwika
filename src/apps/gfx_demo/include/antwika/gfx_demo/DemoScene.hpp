#pragma once

#include <antwika/gfx/IRenderer.hpp>
#include <antwika/gfx/Size.hpp>

namespace antwika::gfx_demo
{

    using antwika::gfx::IRenderer;
    using antwika::gfx::Size;

    /**
     * @brief Draws the demo's picture: a flat background and a row of
     * evenly spaced bars sized relative to the canvas.
     *
     * Stateless and deterministic on purpose. The same canvas size always
     * produces the same drawing calls in the same order, which is what
     * makes the picture assertable against a mock renderer instead of
     * having to be looked at.
     */
    class DemoScene final
    {
    public:
        /**
         * @brief Draw one frame.
         * @param renderer Receives the drawing calls.
         * @param canvas The size of the area being drawn into.
         */
        void draw(IRenderer &renderer, Size canvas) const;
    };

} // namespace antwika::gfx_demo
