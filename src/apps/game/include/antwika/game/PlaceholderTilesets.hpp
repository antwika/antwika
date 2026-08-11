#pragma once

#include <antwika/gfx/Bitmap.hpp>
#include <antwika/tilemap/TerrainClass.hpp>
#include <antwika/tileset/Tileset.hpp>

namespace antwika::game
{

    /**
     * @brief Builds the procedural fallback tileset for a terrain.
     *
     * Ensures: the base layer holds nine sprites, an interior plus
     *          the four edges and four corners, whose edge sockets
     *          face outward and whose one-bit patterns tell the
     *          terrains apart.
     */
    [[nodiscard]] tileset::Tileset placeholderTileset(
        tilemap::TerrainClass terrain);

    /**
     * @brief Builds the procedural 32x8 system sheet.
     *
     * Ensures: the wall band, wall rim, bridge deck, and shade
     *          pieces sit at x 0, 8, 16, and 24, class-coded with
     *          opaque white ink.
     */
    [[nodiscard]] gfx::Bitmap placeholderSystemSheet();

}
