#pragma once

#include <string_view>

#include <antwika/gfx/Bitmap.hpp>

#include "antwika/game/TileAtlas.hpp"

namespace antwika::game
{

    /**
     * @brief Refuse a sheet image that is not the size TileAtlas names.
     *
     * The one check that survived the generator.  While the picture was
     * generated, its size came from the same constants that address it,
     * so the two could not disagree; now that it is drawn by hand, an
     * export at the wrong size is an ordinary mistake with a
     * spectacularly quiet symptom.  gfx::blitIsDrawable() refuses a
     * source rectangle reaching outside its texture *without
     * complaining*, so every sprite past the edge would simply draw
     * nothing and the grid would come up blank.
     *
     * Checked once per sheet at startup rather than per blit, since the
     * answer cannot change while a run is going.
     *
     * @param bitmap The decoded sheet image.
     * @param kind Which of the three sheets it is meant to be.
     * @param name The file it was read from, for the message.
     * @throws antwika::gfx::GfxError If it is not exactly
     * atlasSizeOf(kind), naming the file and both sizes so the message
     * says what to re-export it as.
     */
    void requireAtlasSize(
        const antwika::gfx::Bitmap &bitmap,
        AtlasKind kind,
        std::string_view name);

} // namespace antwika::game
