#pragma once

#include <antwika/gfx/IRenderer.hpp>
#include <antwika/gfx/ITexture.hpp>

#include "antwika/atlas_editor/SceneSnapshot.hpp"

namespace antwika::atlas_editor
{

    using antwika::gfx::IRenderer;
    using antwika::gfx::ITexture;

    /**
     * @brief Draws the sheet, its slot grid and the pixel under the
     * pointer.
     *
     * Stateless and const: everything it draws arrives as a
     * SceneSnapshot and a texture somebody else uploaded, so nothing it
     * does is visible to any other part of the run.
     * It never clears the toolbar's strip either -- the bar is painted
     * over the sheet afterwards, since antwika::gfx has no clipping and
     * paint order is the only depth there is.
     */
    class EditorScene final
    {
    public:
        /**
         * @brief Draw one frame of the sheet.
         * @param renderer Receives the drawing calls.
         * @param snapshot What to draw.
         * @param image The uploaded sheet, or null before anything has
         * been uploaded -- which is an ordinary state, and is what every
         * run under a backend with no textures stays in.
         */
        void draw(
            IRenderer &renderer,
            const SceneSnapshot &snapshot,
            const ITexture *image) const;
    };

} // namespace antwika::atlas_editor
