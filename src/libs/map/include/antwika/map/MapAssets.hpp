#pragma once

#include <map>
#include <string>
#include <string_view>
#include <vector>

#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/ShaderSource.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/tilemap/Tilemap.hpp>
#include <antwika/voxelmap/Voxel.hpp>
#include <antwika/solver/VoxelWeave.hpp>

#include <antwika/map/MapFile.hpp>

namespace antwika::map
{

    [[nodiscard]] std::vector<tilemap::Tile> fallbackTiles(
        const std::vector<voxelmap::FaceRef> &faces,
        const tile::TileRules &rules);

    [[nodiscard]] std::vector<tilemap::Tile> faceTilesFor(
        const Map &map,
        solver::CornerSeams corners,
        std::map<voxelmap::FaceRef, tilemap::Tile> &tileCache);

    [[nodiscard]] gfx::Bitmap readSharedOrBundled(
        const std::string &mapPath,
        std::string_view name,
        std::string_view app);

    [[nodiscard]] gfx::Bitmap loadAtlas(
        const std::string &mapPath,
        std::string_view name,
        gfx::Size tileSize,
        std::string_view app);

    [[nodiscard]] gfx::Bitmap loadCharacterSheet(
        const std::string &mapPath, std::string_view app);

    void writeSharedTexture(
        const gfx::Bitmap &sheetBitmap,
        const std::string &mapPath,
        std::string_view name,
        std::string_view app);

    [[nodiscard]] gfx::ShaderSource loadShader(
        std::string_view stem);

}
