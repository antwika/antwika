#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

#include <antwika/character/Character.hpp>
#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/GfxError.hpp>
#include <antwika/image/PngFile.hpp>
#include <antwika/tilemap/AtlasLayout.hpp>
#include <antwika/voxelmap/Voxel.hpp>

#include <antwika/map/MapAssets.hpp>
#include <antwika/map/MapFile.hpp>

using antwika::tilemap::atlasSize;
using antwika::gfx::Bitmap;
using antwika::tilemap::blankAtlas;
using antwika::character::characterSheetSize;
using antwika::voxelmap::demoCells;
using antwika::map::fallbackTiles;
using antwika::map::faceTilesFor;
using antwika::gfx::GfxError;
using antwika::tilemap::kWallTileSize;
using antwika::map::loadAtlas;
using antwika::map::loadAtlasOrBlank;
using antwika::map::loadCharacterSheet;
using antwika::map::Map;
using antwika::map::readSharedOrBundled;
using antwika::map::sharedTexturePath;
using antwika::map::sidecarPath;
using antwika::gfx::Size;
using antwika::tilemap::Tile;
using antwika::voxelmap::visibleFacesOf;
using antwika::map::writeSharedTexture;

namespace
{

    constexpr std::string_view kAppName = "antwika_map_tests";

    [[nodiscard]] Bitmap markedBitmap(const Size size)
    {
        std::vector<std::uint8_t> pixels(
            static_cast<std::size_t>(size.width) * size.height
                * antwika::gfx::kBytesPerPixel,
            0);

        for (std::size_t index = 0; index < pixels.size(); ++index)
        {
            pixels[index] = static_cast<std::uint8_t>(index % 251);
        }

        return Bitmap{.size = size, .pixels = std::move(pixels)};
    }

    [[nodiscard]] std::string somewhereToMap(const std::string &name)
    {
        const auto mapDirectory = std::filesystem::temp_directory_path()
                                  / "antwika-map-assets" / name / "maps";

        std::filesystem::remove_all(mapDirectory.parent_path());
        std::filesystem::create_directories(mapDirectory);

        return (mapDirectory / "one.json").string();
    }

    TEST(MapAssetsTest, WriteSharedTexture_LaysDownWhatIsReadBack)
    {
        const auto mapPath = somewhereToMap("shared");
        const auto sheetBitmap = markedBitmap(Size{.width = 4, .height = 3});

        writeSharedTexture(sheetBitmap, mapPath, "sheet.png", kAppName);

        EXPECT_TRUE(
            std::filesystem::exists(
                sharedTexturePath(mapPath, "sheet.png")));

        const auto readBitmap =
            readSharedOrBundled(mapPath, "sheet.png", kAppName);

        EXPECT_EQ(readBitmap.size, sheetBitmap.size);
        EXPECT_EQ(readBitmap.pixels, sheetBitmap.pixels);
    }

    TEST(MapAssetsTest, ReadSharedOrBundled_TurnsDownAMissingTexture)
    {
        const auto mapPath = somewhereToMap("missing-shared");

        EXPECT_THROW(
            static_cast<void>(
                readSharedOrBundled(mapPath, "nowhere.png", kAppName)),
            GfxError);
    }

    TEST(MapAssetsTest, LoadAtlas_ReadsTheSidecarBesideTheMap)
    {
        const auto mapPath = somewhereToMap("sidecar-atlas");
        const auto atlasBitmap = markedBitmap(atlasSize(kWallTileSize));

        antwika::image::writePngFile(
            atlasBitmap, sidecarPath(mapPath, "wall.png"), kAppName);

        const auto readBitmap =
            loadAtlas(mapPath, "wall.png", kWallTileSize, kAppName);

        EXPECT_EQ(readBitmap.size, atlasBitmap.size);
        EXPECT_EQ(readBitmap.pixels, atlasBitmap.pixels);
    }

    TEST(MapAssetsTest, LoadAtlas_TurnsDownAMapWithNoAtlasBesideIt)
    {
        const auto mapPath = somewhereToMap("no-atlas");

        EXPECT_THROW(
            static_cast<void>(
                loadAtlas(mapPath, "wall.png", kWallTileSize, kAppName)),
            GfxError);
    }

    TEST(MapAssetsTest, LoadAtlasOrBlank_GivesABlankSheetWithNoSidecar)
    {
        const auto mapPath = somewhereToMap("blank-atlas");
        const auto atlasBitmap =
            loadAtlasOrBlank(mapPath, "wall.png", kWallTileSize, kAppName);

        EXPECT_EQ(atlasBitmap.size, atlasSize(kWallTileSize));
        EXPECT_EQ(atlasBitmap, blankAtlas(kWallTileSize));
    }

    TEST(MapAssetsTest, LoadAtlasOrBlank_TakesTheSidecarWhenThereIsOne)
    {
        const auto mapPath = somewhereToMap("sidecar-over-blank");
        const auto atlasBitmap = markedBitmap(atlasSize(kWallTileSize));

        antwika::image::writePngFile(
            atlasBitmap, sidecarPath(mapPath, "wall.png"), kAppName);

        const auto readBitmap =
            loadAtlasOrBlank(mapPath, "wall.png", kWallTileSize, kAppName);

        EXPECT_EQ(readBitmap.pixels, atlasBitmap.pixels);
    }

    TEST(MapAssetsTest, LoadAtlas_TurnsDownAnAtlasOfAnotherShape)
    {
        const auto mapPath = somewhereToMap("wrong-atlas");

        antwika::image::writePngFile(
            markedBitmap(Size{.width = 8, .height = 8}),
            sidecarPath(mapPath, "wall.png"),
            kAppName);

        EXPECT_THROW(
            static_cast<void>(
                loadAtlas(mapPath, "wall.png", kWallTileSize, kAppName)),
            GfxError);
    }

    TEST(MapAssetsTest, LoadCharacterSheet_ReadsTheSheetTheMapShares)
    {
        const auto mapPath = somewhereToMap("character-sheet");
        const auto sheetBitmap = markedBitmap(characterSheetSize());

        writeSharedTexture(
            sheetBitmap,
            mapPath,
            antwika::character::kCharacterSheet,
            kAppName);

        EXPECT_EQ(
            loadCharacterSheet(mapPath, kAppName).size,
            sheetBitmap.size);
    }

    TEST(MapAssetsTest, LoadCharacterSheet_TurnsDownASheetOfAnotherShape)
    {
        const auto mapPath = somewhereToMap("wrong-character-sheet");

        writeSharedTexture(
            markedBitmap(Size{.width = 6, .height = 6}),
            mapPath,
            antwika::character::kCharacterSheet,
            kAppName);

        EXPECT_THROW(
            static_cast<void>(loadCharacterSheet(mapPath, kAppName)),
            GfxError);
    }

    TEST(MapAssetsTest, FallbackTiles_GivesOneTilePerFaceWithoutRules)
    {
        const auto faces = visibleFacesOf(demoCells());
        const auto tiles = fallbackTiles(faces, antwika::tile::TileRules{});

        EXPECT_EQ(tiles.size(), faces.size());
    }

    TEST(MapAssetsTest, FaceTilesFor_GivesOneTilePerVisibleFace)
    {
        Map map;

        map.voxels = demoCells();

        std::map<antwika::voxelmap::FaceRef, Tile> tileCache;
        const auto tiles = faceTilesFor(
            map, antwika::solver::CornerSeams::Included, tileCache);

        EXPECT_EQ(tiles.size(), visibleFacesOf(map.voxels).size());
    }

}

TEST(MapAssetsTest, AtlasSheets_PairEachNameWithItsTileSize)
{
    ASSERT_EQ(antwika::map::kAtlasSheets.size(), 2U);

    EXPECT_EQ(
        antwika::map::kAtlasSheets[0].name, "atlas-15x9.png");
    EXPECT_EQ(
        antwika::map::kAtlasSheets[0].tileSize,
        antwika::tilemap::kWallTileSize);
    EXPECT_EQ(
        antwika::map::kAtlasSheets[1].name, "atlas-15x12.png");
    EXPECT_EQ(
        antwika::map::kAtlasSheets[1].tileSize,
        antwika::tilemap::kFloorTileSize);
}
