#pragma once

#include <antwika/gfx/Bitmap.hpp>

namespace antwika::game
{

    /**
     * @brief Refuse an atlas image that is not the size TileAtlas names.
     *
     * The one check that survived the generator.  While the picture was
     * generated, its size came from the same constants that address it,
     * so the two could not disagree; now that it is drawn by hand, an
     * export at the wrong size is an ordinary mistake with a
     * spectacularly quiet symptom.  gfx::blitIsDrawable() refuses a
     * source rectangle reaching outside its texture *without
     * complaining*, so every tile past the edge would simply draw
     * nothing and the grid would come up blank.
     *
     * Checked once at startup rather than per blit, since the answer
     * cannot change while a run is going.
     *
     * @param bitmap The decoded atlas image.
     * @throws antwika::gfx::GfxError If it is not exactly kAtlasSize,
     * naming both sizes so the message says what to re-export it as.
     */
    void requireAtlasSize(const antwika::gfx::Bitmap &bitmap);

} // namespace antwika::game
