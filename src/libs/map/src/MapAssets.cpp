#include "antwika/map/MapAssets.hpp"

#include <filesystem>
#include <system_error>
#include <fstream>
#include <utility>

#include <antwika/tilemap/AtlasLayout.hpp>
#include <antwika/io/AssetPath.hpp>
#include <antwika/image/PngFile.hpp>
#include <antwika/gfx/GfxError.hpp>
#include <antwika/gfx/Math3D.hpp>
#include <antwika/gfx/ShaderReader.hpp>
#include <antwika/character/Character.hpp>

namespace antwika::map
{

    gfx::Bitmap readSharedOrBundled(
        const std::string &mapPath,
        const std::string_view name,
        const std::string_view app)
    {
        const auto sharedPath = sharedTexturePath(mapPath, name);

        if (std::filesystem::exists(sharedPath))
        {
            return image::readPngFile(sharedPath, app);
        }

        return image::readPngFile(
            io::assetPath(std::string(name)), app);
    }

    std::vector<tilemap::Tile> fallbackTiles(
        const std::vector<voxelmap::FaceRef> &faces,
        const tile::TileRules &rules)
    {
        std::map<std::pair<tilemap::Atlas, voxel::Kind>, tilemap::Tile>
            firstTiles;

        for (const auto &rule : rules.allRules())
        {
            const auto want =
                std::pair{rule.tile.atlas, rules.kindOf(rule.tile)};
            const auto foundTile = firstTiles.find(want);

            if (foundTile == firstTiles.end() || rule.tile < foundTile->second)
            {
                firstTiles.insert_or_assign(want, rule.tile);
            }
        }

        const auto defaultTileList = voxelmap::defaultTiles(faces);

        std::vector<tilemap::Tile> tiles;

        tiles.reserve(faces.size());

        for (std::size_t index = 0; index < faces.size(); ++index)
        {
            const auto want = std::pair{
                gfx::Vec3(voxelmap::faceNormal(faces[index].side)).y != 0.0F
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

    std::vector<tilemap::Tile> faceTilesFor(
        const Map &map,
        const solver::CornerSeams corners,
        std::map<voxelmap::FaceRef, tilemap::Tile> &tileCache)
    {
        const auto faces = voxelmap::visibleFacesOf(map.voxels);
        const auto solvedTiles = solver::solveTiles(faces, map.rules, corners);

        if (solvedTiles.tiles.has_value())
        {
            tileCache.clear();

            for (std::size_t index = 0; index < faces.size(); ++index)
            {
                tileCache.emplace(faces[index], (*solvedTiles.tiles)[index]);
            }

            return *solvedTiles.tiles;
        }

        const auto fallbackTileList = fallbackTiles(faces, map.rules);

        std::vector<tilemap::Tile> tiles;

        tiles.reserve(faces.size());

        for (std::size_t index = 0; index < faces.size(); ++index)
        {
            const auto was = tileCache.find(faces[index]);

            tiles.push_back(
                was == tileCache.end() ? fallbackTileList[index]
                     : was->second);
        }

        return tiles;
    } // GCOVR_EXCL_LINE

    gfx::Bitmap loadAtlas(
        const std::string &mapPath,
        const std::string_view name,
        const gfx::Size tileSize,
        const std::string_view app)
    {
        const auto sidecarFilePath = sidecarPath(mapPath, name);

        if (!std::filesystem::exists(sidecarFilePath))
        {
            throw gfx::GfxError(
                "the map has no atlas at " + sidecarFilePath);
        }

        auto atlas = image::readPngFile(sidecarFilePath, app);
        const auto wantedSize = tilemap::atlasSize(tileSize);

        if (atlas.size != wantedSize)
        {
            throw gfx::GfxError(
                std::string(name) + " is not the atlas the voxels "
                "were built for");
        }

        return atlas;
    } // GCOVR_EXCL_LINE

    std::array<gfx::Bitmap, kAtlasSheetCount> loadAtlasPair(
        const std::string &mapPath, const std::string_view app)
    {
        return {
            loadAtlas(
                mapPath,
                kAtlasSheets[0].name,
                kAtlasSheets[0].tileSize,
                app),
            loadAtlas(
                mapPath,
                kAtlasSheets[1].name,
                kAtlasSheets[1].tileSize,
                app)};
    }

    std::array<gfx::Bitmap, kAtlasSheetCount> loadAtlasPairOrBlank(
        const std::string &mapPath, const std::string_view app)
    {
        return {
            loadAtlasOrBlank(
                mapPath,
                kAtlasSheets[0].name,
                kAtlasSheets[0].tileSize,
                app),
            loadAtlasOrBlank(
                mapPath,
                kAtlasSheets[1].name,
                kAtlasSheets[1].tileSize,
                app)};
    }

    gfx::Bitmap loadAtlasOrBlank(
        const std::string &mapPath,
        const std::string_view name,
        const gfx::Size tileSize,
        const std::string_view app)
    {
        if (!std::filesystem::exists(sidecarPath(mapPath, name)))
        {
            return tilemap::blankAtlas(tileSize);
        }

        return loadAtlas(mapPath, name, tileSize, app);
    } // GCOVR_EXCL_LINE

    void writeSharedTexture(
        const gfx::Bitmap &sheetBitmap,
        const std::string &mapPath,
        const std::string_view name,
        const std::string_view app)
    {
        const auto sharedPath = sharedTexturePath(mapPath, name);
        std::error_code errorCode;

        std::filesystem::create_directories(
            std::filesystem::path(sharedPath).parent_path(), errorCode);
        image::writePngFile(sheetBitmap, sharedPath, app);
    }

    gfx::Bitmap loadCharacterSheet(
        const std::string &mapPath, const std::string_view app)
    {
        auto sheet =
            readSharedOrBundled(mapPath, character::kCharacterSheet, app);

        if (sheet.size != character::characterSheetSize())
        {
            throw gfx::GfxError(
                "the character sheet is not the shape a character "
                "is drawn on");
        }

        return sheet;
    } // GCOVR_EXCL_LINE

    gfx::ShaderSource loadShader(const std::string_view stem)
    {
        std::ifstream vertex(
            io::assetPath(std::string(stem) + ".vert"));
        std::ifstream fragment(
            io::assetPath(std::string(stem) + ".frag"));

        return gfx::ShaderReader().read(vertex, fragment);
    } // GCOVR_EXCL_LINE

}
