#include "antwika/assets/MapAssets.hpp"

#include <filesystem>
#include <system_error>
#include <utility>

#include <antwika/tilemap/AtlasLayout.hpp>
#include <antwika/io/AssetPath.hpp>
#include <antwika/image/PngFile.hpp>
#include <antwika/gfx/GfxError.hpp>
#include <antwika/gfx/Math3D.hpp>
#include <antwika/character/Character.hpp>

namespace antwika::assets
{

    gfx::Bitmap getReadSharedOrBundled(
        const std::string &mapPath,
        const std::string_view name,
        const std::string_view app)
    {
        const auto sharedPath = map::getSharedTexturePath(mapPath, name);

        if (std::filesystem::exists(sharedPath))
        {
            return image::getReadPngFile(sharedPath, app);
        }

        return image::getReadPngFile(
            io::getAssetPath(std::string(name)), app);
    }

    std::vector<tilemap::Tile> getFallbackTiles(
        const std::vector<voxelmap::FaceRef> &faces,
        const tile::TileRules &rules)
    {
        std::map<std::pair<tilemap::Atlas, voxel::Kind>, tilemap::Tile>
            firstTiles;

        for (const auto &rule : rules.getAllRules())
        {
            const auto want =
                std::pair{rule.tile.atlas, rules.kindOf(rule.tile)};
            const auto foundTile = firstTiles.find(want);

            if (foundTile == firstTiles.end() || rule.tile < foundTile->second)
            {
                firstTiles.insert_or_assign(want, rule.tile);
            }
        }

        const auto defaultTileList = voxelmap::getDefaultTiles(faces);

        std::vector<tilemap::Tile> tiles;

        tiles.reserve(faces.size());

        for (std::size_t index = 0; index < faces.size(); ++index)
        {
            const auto want = std::pair{
                gfx::Vec3(voxelmap::getFaceNormal(faces[index].side)).y != 0.0F
                    ? tilemap::Atlas::Floor
                    : tilemap::Atlas::Wall,
                faces[index].cell.material.kind};
            const auto foundTile = firstTiles.find(want);

            tiles.push_back(
                foundTile == firstTiles.end() ? defaultTileList[index]
                           : foundTile->second);
        }

        return tiles;
    } // GCOVR_EXCL_LINE

    WovenTiles faceTilesFor(
        const std::vector<voxelmap::FaceRef> &faces,
        const tile::TileRules &rules,
        const solver::CornerSeams corners,
        std::map<voxelmap::FaceRef, tilemap::Tile> &tileCache)
    {
        auto solvedTiles = solver::getSolveTiles(faces, rules, corners);

        if (solvedTiles.tiles.has_value())
        {
            tileCache.clear();

            for (std::size_t index = 0; index < faces.size(); ++index)
            {
                tileCache.emplace(faces[index], (*solvedTiles.tiles)[index]);
            }

            return WovenTiles{
                .tiles = *solvedTiles.tiles, .solve = std::move(solvedTiles)};
        }

        const auto fallbackTileList = getFallbackTiles(faces, rules);

        std::vector<tilemap::Tile> tiles;

        tiles.reserve(faces.size());

        for (std::size_t index = 0; index < faces.size(); ++index)
        {
            const auto was = tileCache.find(faces[index]);

            tiles.push_back(
                was == tileCache.end() ? fallbackTileList[index]
                     : was->second);
        }

        return WovenTiles{
            .tiles = std::move(tiles), .solve = std::move(solvedTiles)};
    } // GCOVR_EXCL_LINE

    gfx::Bitmap getLoadAtlas(
        const std::string &mapPath,
        const std::string_view name,
        const gfx::Size tileSize,
        const std::string_view app)
    {
        const auto sidecarFilePath = map::getSidecarPath(mapPath, name);

        if (!std::filesystem::exists(sidecarFilePath))
        {
            throw gfx::GfxError(
                "the map has no atlas at " + sidecarFilePath);
        }

        auto atlas = image::getReadPngFile(sidecarFilePath, app);
        const auto wantedSize = tilemap::getAtlasSize(tileSize);

        if (atlas.size != wantedSize)
        {
            throw gfx::GfxError(
                std::string(name) + " is not the atlas the voxels "
                "were built for");
        }

        return atlas;
    } // GCOVR_EXCL_LINE

    std::array<gfx::Bitmap, kAtlasSheetCount> getLoadAtlasPair(
        const std::string &mapPath, const std::string_view app)
    {
        return {
            getLoadAtlas(
                mapPath,
                kAtlasSheets[0].name,
                kAtlasSheets[0].tileSize,
                app),
            getLoadAtlas(
                mapPath,
                kAtlasSheets[1].name,
                kAtlasSheets[1].tileSize,
                app)};
    }

    std::array<gfx::Bitmap, kAtlasSheetCount> getLoadAtlasPairOrBlank(
        const std::string &mapPath, const std::string_view app)
    {
        return {
            getLoadAtlasOrBlank(
                mapPath,
                kAtlasSheets[0].name,
                kAtlasSheets[0].tileSize,
                app),
            getLoadAtlasOrBlank(
                mapPath,
                kAtlasSheets[1].name,
                kAtlasSheets[1].tileSize,
                app)};
    }

    gfx::Bitmap getLoadAtlasOrBlank(
        const std::string &mapPath,
        const std::string_view name,
        const gfx::Size tileSize,
        const std::string_view app)
    {
        if (!std::filesystem::exists(map::getSidecarPath(mapPath, name)))
        {
            return tilemap::getBlankAtlas(tileSize);
        }

        return getLoadAtlas(mapPath, name, tileSize, app);
    } // GCOVR_EXCL_LINE

    void writeSharedTexture(
        const gfx::Bitmap &sheetBitmap,
        const std::string &mapPath,
        const std::string_view name,
        const std::string_view app)
    {
        const auto sharedPath = map::getSharedTexturePath(mapPath, name);
        std::error_code errorCode;

        std::filesystem::create_directories(
            std::filesystem::path(sharedPath).parent_path(), errorCode);
        image::writePngFile(sheetBitmap, sharedPath, app);
    }

    gfx::Bitmap getLoadCharacterSheet(
        const std::string &mapPath, const std::string_view app)
    {
        auto sheet =
            getReadSharedOrBundled(mapPath, character::kCharacterSheet, app);

        if (sheet.size != character::getCharacterSheetSize())
        {
            throw gfx::GfxError(
                "the character sheet is not the shape a character "
                "is drawn on");
        }

        return sheet;
    } // GCOVR_EXCL_LINE

}
