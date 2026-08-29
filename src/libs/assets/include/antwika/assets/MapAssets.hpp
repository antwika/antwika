#pragma once

#include <array>
#include <map>
#include <string>
#include <string_view>
#include <vector>

#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/tilemap/AtlasLayout.hpp>
#include <antwika/tilemap/Tilemap.hpp>
#include <antwika/map/MapFile.hpp>
#include <antwika/solver/VoxelWeave.hpp>
#include <antwika/voxelmap/Voxel.hpp>

#include "antwika/assets/WovenTiles.hpp"

namespace antwika::assets
{

    [[nodiscard]] std::vector<tilemap::Tile> getFallbackTiles(
        const std::vector<voxelmap::FaceRef> &faces,
        const tile::TileRules &rules);

    [[nodiscard]] WovenTiles faceTilesFor(
        const std::vector<voxelmap::FaceRef> &faces,
        const tile::TileRules &rules,
        solver::CornerSeams corners,
        std::map<voxelmap::FaceRef, tilemap::Tile> &tileCache);

    [[nodiscard]] gfx::Bitmap getReadSharedOrBundled(
        const std::string &mapPath,
        std::string_view name,
        std::string_view app);

    struct AtlasSheet final
    {
        std::string_view name;

        gfx::Size tileSize;
    };

    inline constexpr std::size_t kAtlasSheetCount = 2;

    inline constexpr std::array<AtlasSheet, kAtlasSheetCount>
        kAtlasSheets{
            AtlasSheet{
                .name = "atlas-15x9.png",
                .tileSize = tilemap::kWallTileSize},
            AtlasSheet{
                .name = "atlas-15x12.png",
                .tileSize = tilemap::kFloorTileSize}};

    [[nodiscard]] gfx::Bitmap getLoadAtlas(
        const std::string &mapPath,
        std::string_view name,
        gfx::Size tileSize,
        std::string_view app);

    [[nodiscard]] std::array<gfx::Bitmap, kAtlasSheetCount> getLoadAtlasPair(
        const std::string &mapPath, std::string_view app);

    [[nodiscard]] std::array<gfx::Bitmap, kAtlasSheetCount>
    getLoadAtlasPairOrBlank(
        const std::string &mapPath, std::string_view app);

    [[nodiscard]] gfx::Bitmap getLoadAtlasOrBlank(
        const std::string &mapPath,
        std::string_view name,
        gfx::Size tileSize,
        std::string_view app);

    [[nodiscard]] gfx::Bitmap getLoadCharacterSheet(
        const std::string &mapPath, std::string_view app);

    void writeSharedTexture(
        const gfx::Bitmap &sheetBitmap,
        const std::string &mapPath,
        std::string_view name,
        std::string_view app);

}
