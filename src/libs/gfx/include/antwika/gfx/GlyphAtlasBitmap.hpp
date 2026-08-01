#pragma once

#include <antwika/font/GlyphAtlas.hpp>

#include "antwika/gfx/Bitmap.hpp"

namespace antwika::gfx
{

    /**
     * @brief Expand a packed coverage mask into pixels a renderer can
     * upload.
     *
     * This is the one seam antwika::font's Coverage was shaped for, and
     * it is deliberately four lines of arithmetic rather than anything
     * either library had to give up to have it.  A glyph carries one
     * channel -- how much ink landed on a pixel -- so the mask becomes
     * white with the coverage in alpha, and drawTexture()'s tint is
     * what picks the colour.  One uploaded atlas therefore serves every
     * colour of text an application ever draws.
     *
     * The dependency runs this way round and cannot run the other:
     * antwika::font names no module of this project at all, so
     * gfx -> font is acyclic by construction rather than by agreement.
     *
     * @param atlas The packed mask to expand.
     * @return Straight RGBA, the same size as the mask, ready for
     * IRenderer::createTexture().
     * @throws GfxError If the atlas has no mask in it, or if its mask
     * holds fewer samples than its size claims.
     */
    [[nodiscard]] Bitmap glyphAtlasBitmap(const font::GlyphAtlas &atlas);

} // namespace antwika::gfx
