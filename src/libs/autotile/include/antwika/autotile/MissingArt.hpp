#pragma once

#include <antwika/tileset/Atlas.hpp>
#include <antwika/tileset/Tileset.hpp>

#include "antwika/autotile/TileDraw.hpp"

namespace antwika::autotile
{

    /**
     * @brief Whether a planned draw has no art an artist drew.
     *
     * @param draw The draw to judge.
     * @param set The tileset bound to the draw's terrain.
     * @param index The atlas index the bound texture was baked from.
     * @return Whether the draw must render as a missing-art marker.
     *
     * Ensures: DrawKind::WallRim, DrawKind::WallBand and
     *          DrawKind::BridgeDeck are missing whatever the tileset
     *          holds, because no tileset can supply them.
     * Ensures: DrawKind::Shade is never missing, because the lighting
     *          pass draws it rather than an artist.
     * Ensures: a DrawKind::Sprite is missing when its row is past the
     *          baked atlas, when the tileset holds no sprite at that
     *          row, or when the drawn frame is blank.
     */
    [[nodiscard]] bool artMissing(
        const TileDraw &draw,
        const tileset::Tileset &set,
        const tileset::AtlasIndex &index);

}
