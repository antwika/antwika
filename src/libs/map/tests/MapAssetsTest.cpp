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

using antwika::tilemap::getAtlasSize;
using antwika::gfx::Bitmap;
using antwika::tilemap::getBlankAtlas;
using antwika::character::getCharacterSheetSize;
using antwika::voxelmap::getDemoCells;
using antwika::map::getFallbackTiles;
using antwika::map::faceTilesFor;
using antwika::gfx::GfxError;
using antwika::tilemap::kWallTileSize;
using antwika::map::getLoadAtlas;
using antwika::map::getLoadAtlasOrBlank;
using antwika::map::getLoadCharacterSheet;
using antwika::map::Map;
using antwika::map::getReadSharedOrBundled;
using antwika::map::getSharedTexturePath;
using antwika::map::getSidecarPath;
using antwika::gfx::Size;
using antwika::tilemap::Tile;
using antwika::voxelmap::visibleFacesOf;
using antwika::map::writeSharedTexture;

namespace
{

    constexpr std::string_view kAppName = "antwika_map_tests";

    [[nodiscard]] Bitmap getMarkedBitmap(const Size size)
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

    [[nodiscard]] std::string getSomewhereToMap(const std::string &name)
    {
        const auto mapDirectory = std::filesystem::temp_directory_path()
                                  / "antwika-map-assets" / name / "maps";

        std::filesystem::remove_all(mapDirectory.parent_path());
        std::filesystem::create_directories(mapDirectory);

        return (mapDirectory / "one.json").string();
    }

    TEST(MapAssetsTest, WriteSharedTexture_LaysDownWhatIsReadBack)
    {
        const auto mapPath = getSomewhereToMap("shared");
        const auto sheetBitmap = getMarkedBitmap(Size{.width = 4, .height = 3});

        writeSharedTexture(sheetBitmap, mapPath, "sheet.png", kAppName);

        EXPECT_TRUE(
            std::filesystem::exists(
                getSharedTexturePath(mapPath, "sheet.png")));

        const auto readBitmap =
            getReadSharedOrBundled(mapPath, "sheet.png", kAppName);

        EXPECT_EQ(readBitmap.size, sheetBitmap.size);
        EXPECT_EQ(readBitmap.pixels, sheetBitmap.pixels);
    }

    TEST(MapAssetsTest, ReadSharedOrBundled_TurnsDownAMissingTexture)
    {
        const auto mapPath = getSomewhereToMap("missing-shared");

        EXPECT_THROW(
            static_cast<void>(
                getReadSharedOrBundled(mapPath, "nowhere.png", kAppName)),
            GfxError);
    }

    TEST(MapAssetsTest, LoadAtlas_ReadsTheSidecarBesideTheMap)
    {
        const auto mapPath = getSomewhereToMap("sidecar-atlas");
        const auto atlasBitmap = getMarkedBitmap(getAtlasSize(kWallTileSize));

        antwika::image::writePngFile(
            atlasBitmap, getSidecarPath(mapPath, "wall.png"), kAppName);

        const auto readBitmap =
            getLoadAtlas(mapPath, "wall.png", kWallTileSize, kAppName);

        EXPECT_EQ(readBitmap.size, atlasBitmap.size);
        EXPECT_EQ(readBitmap.pixels, atlasBitmap.pixels);
    }

    TEST(MapAssetsTest, LoadAtlas_TurnsDownAMapWithNoAtlasBesideIt)
    {
        const auto mapPath = getSomewhereToMap("no-atlas");

        EXPECT_THROW(
            static_cast<void>(
                getLoadAtlas(mapPath, "wall.png", kWallTileSize, kAppName)),
            GfxError);
    }

    TEST(MapAssetsTest, LoadAtlasOrBlank_GivesABlankSheetWithNoSidecar)
    {
        const auto mapPath = getSomewhereToMap("blank-atlas");
        const auto atlasBitmap =
            getLoadAtlasOrBlank(mapPath, "wall.png", kWallTileSize, kAppName);

        EXPECT_EQ(atlasBitmap.size, getAtlasSize(kWallTileSize));
        EXPECT_EQ(atlasBitmap, getBlankAtlas(kWallTileSize));
    }

    TEST(MapAssetsTest, LoadAtlasOrBlank_TakesTheSidecarWhenThereIsOne)
    {
        const auto mapPath = getSomewhereToMap("sidecar-over-blank");
        const auto atlasBitmap = getMarkedBitmap(getAtlasSize(kWallTileSize));

        antwika::image::writePngFile(
            atlasBitmap, getSidecarPath(mapPath, "wall.png"), kAppName);

        const auto readBitmap =
            getLoadAtlasOrBlank(mapPath, "wall.png", kWallTileSize, kAppName);

        EXPECT_EQ(readBitmap.pixels, atlasBitmap.pixels);
    }

    TEST(MapAssetsTest, LoadAtlas_TurnsDownAnAtlasOfAnotherShape)
    {
        const auto mapPath = getSomewhereToMap("wrong-atlas");

        antwika::image::writePngFile(
            getMarkedBitmap(Size{.width = 8, .height = 8}),
            getSidecarPath(mapPath, "wall.png"),
            kAppName);

        EXPECT_THROW(
            static_cast<void>(
                getLoadAtlas(mapPath, "wall.png", kWallTileSize, kAppName)),
            GfxError);
    }

    TEST(MapAssetsTest, LoadCharacterSheet_ReadsTheSheetTheMapShares)
    {
        const auto mapPath = getSomewhereToMap("character-sheet");
        const auto sheetBitmap = getMarkedBitmap(getCharacterSheetSize());

        writeSharedTexture(
            sheetBitmap,
            mapPath,
            antwika::character::kCharacterSheet,
            kAppName);

        EXPECT_EQ(
            getLoadCharacterSheet(mapPath, kAppName).size,
            sheetBitmap.size);
    }

    TEST(MapAssetsTest, LoadCharacterSheet_TurnsDownASheetOfAnotherShape)
    {
        const auto mapPath = getSomewhereToMap("wrong-character-sheet");

        writeSharedTexture(
            getMarkedBitmap(Size{.width = 6, .height = 6}),
            mapPath,
            antwika::character::kCharacterSheet,
            kAppName);

        EXPECT_THROW(
            static_cast<void>(getLoadCharacterSheet(mapPath, kAppName)),
            GfxError);
    }

    TEST(MapAssetsTest, FallbackTiles_GivesOneTilePerFaceWithoutRules)
    {
        const auto faces = visibleFacesOf(getDemoCells());
        const auto tiles = getFallbackTiles(faces, antwika::tile::TileRules{});

        EXPECT_EQ(tiles.size(), faces.size());
    }

    TEST(MapAssetsTest, FaceTilesFor_GivesOneTilePerVisibleFace)
    {
        Map map;

        map.voxels = getDemoCells();

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
