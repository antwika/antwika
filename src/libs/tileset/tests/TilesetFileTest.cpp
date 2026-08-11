#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <ios>
#include <string>
#include <vector>

#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/PngWriter.hpp>
#include <antwika/testing/ScratchPath.hpp>
#include <antwika/tilemap/TerrainClass.hpp>
#include <antwika/tileset/PixelClass.hpp>
#include <antwika/tileset/Sprite.hpp>
#include <antwika/tileset/Tileset.hpp>
#include <antwika/tileset/TilesetError.hpp>
#include <antwika/tileset/TilesetFile.hpp>

using antwika::gfx::Bitmap;
using antwika::testing::ScratchDirectory;
using antwika::tilemap::TerrainClass;
using antwika::tileset::addLayer;
using antwika::tileset::addSprite;
using antwika::tileset::Layer;
using antwika::tileset::layerBitmapOf;
using antwika::tileset::listTilesets;
using antwika::tileset::loadTileset;
using antwika::tileset::loadTilesetLibrary;
using antwika::tileset::PixelClass;
using antwika::tileset::readLayerBitmap;
using antwika::tileset::saveTileset;
using antwika::tileset::Tileset;
using antwika::tileset::TilesetError;

namespace
{
    /**
     * @brief A file that opens for writing and fails every write.
     */
    constexpr auto kFullDevice = "/dev/full";

    [[nodiscard]] Tileset inkedTileset()
    {
        Tileset set;
        set.name = "rustwall";
        set.terrain = TerrainClass::Wall;

        auto &first = addSprite(set, 0);
        first.frames[0].pixels[0] = PixelClass::Ink;
        first.frames[0].pixels[9] = PixelClass::Paper;

        static_cast<void>(addLayer(set, "moss"));

        auto &tuft = addSprite(set, 1);
        tuft.frameCount = 2;
        tuft.frames[1].pixels[63] = PixelClass::Ink;
        tuft.on = {0};

        static_cast<void>(addLayer(set, "vine"));

        return set;
    }

    void setPixelBytes(
        Bitmap &bitmap,
        const std::uint32_t x,
        const std::uint32_t y,
        const std::uint8_t red,
        const std::uint8_t green,
        const std::uint8_t blue,
        const std::uint8_t alpha)
    {
        const auto offset = static_cast<std::size_t>(
                                y * bitmap.size.width + x)
                            * antwika::gfx::kBytesPerPixel;

        bitmap.pixels[offset] = red;
        bitmap.pixels[offset + 1] = green;
        bitmap.pixels[offset + 2] = blue;
        bitmap.pixels[offset + 3] = alpha;
    }

    [[nodiscard]] Bitmap blankLayerBitmap(const std::uint32_t rows)
    {
        return Bitmap{
            .size = {.width = 32, .height = 8 * rows},
            .pixels = std::vector<std::uint8_t>(
                static_cast<std::size_t>(32) * 8 * rows
                    * antwika::gfx::kBytesPerPixel,
                0)};
    }
}

TEST(TilesetFileTest, LayerBitmapOf_CodesInkPaperAndBlankPixels)
{
    const auto set = inkedTileset();

    const auto bitmap = layerBitmapOf(set.layers[0]);

    ASSERT_EQ(bitmap.size.width, 32U);
    ASSERT_EQ(bitmap.size.height, 8U);
    EXPECT_EQ(bitmap.pixels[0], 255);
    EXPECT_EQ(bitmap.pixels[1], 255);
    EXPECT_EQ(bitmap.pixels[2], 255);
    EXPECT_EQ(bitmap.pixels[3], 255);

    const auto paperOffset = static_cast<std::size_t>(
                                 1U * bitmap.size.width + 1U)
                             * antwika::gfx::kBytesPerPixel;

    EXPECT_EQ(bitmap.pixels[paperOffset], 128);
    EXPECT_EQ(bitmap.pixels[paperOffset + 3], 255);
    EXPECT_EQ(bitmap.pixels[7], 0);
}

TEST(TilesetFileTest, LayerBitmapOf_ClearsFrameSlotsPastFrameCount)
{
    Tileset set;

    auto &sprite = addSprite(set, 0);
    sprite.frames[1].pixels[0] = PixelClass::Ink;

    const auto bitmap = layerBitmapOf(set.layers[0]);

    const auto slotOffset = static_cast<std::size_t>(8)
                            * antwika::gfx::kBytesPerPixel;

    EXPECT_EQ(bitmap.pixels[slotOffset + 3], 0);
}

TEST(TilesetFileTest, ReadLayerBitmap_NormalizesOpaqueByLuminance)
{
    Tileset set;
    static_cast<void>(addSprite(set, 0));

    auto bitmap = blankLayerBitmap(1);
    setPixelBytes(bitmap, 0, 0, 200, 200, 200, 255);
    setPixelBytes(bitmap, 1, 0, 100, 100, 100, 255);
    setPixelBytes(bitmap, 2, 0, 255, 0, 0, 255);

    readLayerBitmap(set.layers[0], bitmap);

    const auto &pixels = set.layers[0].sprites[0].frames[0].pixels;

    EXPECT_EQ(pixels[0], PixelClass::Ink);
    EXPECT_EQ(pixels[1], PixelClass::Paper);
    EXPECT_EQ(pixels[2], PixelClass::Paper);
    EXPECT_EQ(pixels[3], PixelClass::Blank);
}

TEST(TilesetFileTest, ReadLayerBitmap_RoundTripsLayerBitmapOf)
{
    const auto set = inkedTileset();
    auto reloaded = inkedTileset();

    for (auto &sprite : reloaded.layers[0].sprites)
    {
        sprite.frames = {};
    }

    readLayerBitmap(
        reloaded.layers[0], layerBitmapOf(set.layers[0]));

    EXPECT_EQ(
        reloaded.layers[0].sprites[0].frames[0].pixels[0],
        PixelClass::Ink);
    EXPECT_EQ(
        reloaded.layers[0].sprites, set.layers[0].sprites);
}

TEST(TilesetFileTest, ReadLayerBitmap_IgnoresSlotsPastFrameCount)
{
    Tileset set;
    static_cast<void>(addSprite(set, 0));

    auto bitmap = blankLayerBitmap(1);
    setPixelBytes(bitmap, 8, 0, 255, 255, 255, 255);

    readLayerBitmap(set.layers[0], bitmap);

    EXPECT_EQ(
        set.layers[0].sprites[0].frames[1].pixels[0],
        PixelClass::Blank);
}

TEST(TilesetFileTest, ReadLayerBitmap_ClampsAFrameCountPastTheArray)
{
    Tileset set;
    addSprite(set, 0).frameCount = 9;

    auto bitmap = blankLayerBitmap(1);
    setPixelBytes(bitmap, 16, 0, 255, 255, 255, 255);

    readLayerBitmap(set.layers[0], bitmap);

    EXPECT_EQ(
        set.layers[0].sprites[0].frames[2].pixels[0],
        PixelClass::Ink);
}

TEST(TilesetFileTest, ReadLayerBitmap_RejectsAWrongShape)
{
    Tileset set;
    static_cast<void>(addSprite(set, 0));

    EXPECT_THROW(
        readLayerBitmap(set.layers[0], blankLayerBitmap(2)),
        TilesetError);
}

TEST(TilesetFileTest, LoadTileset_ReturnsWhatSaveTilesetWrote)
{
    const ScratchDirectory scratch("tileset.");
    const auto set = inkedTileset();
    const auto directory = scratch.path() / "rustwall";

    saveTileset(directory, set);

    EXPECT_EQ(loadTileset(directory), set);
}

TEST(TilesetFileTest, SaveTileset_WritesNoImageForASpritelessLayer)
{
    const ScratchDirectory scratch("tileset.");
    const auto directory = scratch.path() / "rustwall";

    saveTileset(directory, inkedTileset());

    EXPECT_TRUE(
        std::filesystem::is_regular_file(
            directory / "tileset.json"));
    EXPECT_TRUE(
        std::filesystem::is_regular_file(
            directory / "layer-0.png"));
    EXPECT_FALSE(
        std::filesystem::exists(directory / "layer-2.png"));
}

TEST(TilesetFileTest, SaveTileset_RemovesTheImageOfANowEmptyLayer)
{
    const ScratchDirectory scratch("tileset.");
    auto set = inkedTileset();
    const auto directory = scratch.path() / "rustwall";

    saveTileset(directory, set);
    set.layers[1].sprites.clear();
    saveTileset(directory, set);

    EXPECT_FALSE(
        std::filesystem::exists(directory / "layer-1.png"));
}

TEST(TilesetFileTest, SaveTileset_RejectsAnUncreatableDirectory)
{
    const ScratchDirectory scratch("tileset.");
    scratch.write("occupied", "a file where a directory should go");

    EXPECT_THROW(
        saveTileset(
            scratch.path() / "occupied" / "rustwall",
            inkedTileset()),
        TilesetError);
}

TEST(TilesetFileTest, SaveTileset_RejectsAnUnopenableTilesetFile)
{
    const ScratchDirectory scratch("tileset.");
    const auto directory = scratch.path() / "rustwall";
    std::filesystem::create_directories(
        directory / "tileset.json");

    EXPECT_THROW(
        saveTileset(directory, inkedTileset()), TilesetError);
}

TEST(TilesetFileTest, SaveTileset_RejectsAnUnopenableLayerImage)
{
    const ScratchDirectory scratch("tileset.");
    const auto directory = scratch.path() / "rustwall";
    std::filesystem::create_directories(
        directory / "layer-0.png");

    EXPECT_THROW(
        saveTileset(directory, inkedTileset()), TilesetError);
}

TEST(TilesetFileTest, LoadTileset_RejectsAMissingTilesetFile)
{
    const ScratchDirectory scratch("tileset.");

    EXPECT_THROW(
        (void)loadTileset(scratch.path() / "absent"),
        TilesetError);
}

TEST(TilesetFileTest, LoadTileset_RejectsAMissingLayerImage)
{
    const ScratchDirectory scratch("tileset.");
    const auto directory = scratch.path() / "rustwall";

    saveTileset(directory, inkedTileset());
    std::filesystem::remove(directory / "layer-0.png");

    EXPECT_THROW((void)loadTileset(directory), TilesetError);
}

TEST(TilesetFileTest, LoadTileset_RejectsALayerImageOfAWrongShape)
{
    const ScratchDirectory scratch("tileset.");
    const auto directory = scratch.path() / "rustwall";

    saveTileset(directory, inkedTileset());

    std::ofstream out(
        directory / "layer-0.png", std::ios::binary);
    antwika::gfx::PngWriter{}.write(blankLayerBitmap(2), out);
    out.close();

    EXPECT_THROW((void)loadTileset(directory), TilesetError);
}

TEST(TilesetFileTest, LoadTileset_RejectsAnUndecodableLayerImage)
{
    const ScratchDirectory scratch("tileset.");
    const auto directory = scratch.path() / "rustwall";

    saveTileset(directory, inkedTileset());

    std::ofstream out(
        directory / "layer-0.png", std::ios::binary);
    out << "not a png";
    out.close();

    EXPECT_THROW((void)loadTileset(directory), TilesetError);
}

TEST(TilesetFileTest, ListTilesets_NamesSortedTilesetDirectories)
{
    const ScratchDirectory scratch("tileset.");

    saveTileset(scratch.path() / "zeta", inkedTileset());
    saveTileset(scratch.path() / "alpha", inkedTileset());
    std::filesystem::create_directories(scratch.path() / "plain");
    scratch.write("note.txt", "not a tileset");

    EXPECT_EQ(
        listTilesets(scratch.path()),
        (std::vector<std::string>{"alpha", "zeta"}));
}

TEST(TilesetFileTest, ListTilesets_ListsNothingForAMissingDirectory)
{
    const ScratchDirectory scratch("tileset.");

    EXPECT_TRUE(listTilesets(scratch.path() / "absent").empty());
}

TEST(TilesetFileTest, LoadTilesetLibrary_SortsByTilesetName)
{
    const ScratchDirectory scratch("tileset.");
    auto zeta = inkedTileset();
    zeta.name = "zeta";
    auto alpha = inkedTileset();
    alpha.name = "alpha";

    saveTileset(scratch.path() / "a", zeta);
    saveTileset(scratch.path() / "b", alpha);

    const auto library = loadTilesetLibrary(scratch.path());

    ASSERT_EQ(library.size(), 2U);
    EXPECT_EQ(library[0].name, "alpha");
    EXPECT_EQ(library[1].name, "zeta");
}

TEST(TilesetFileTest, LoadTilesetLibrary_SkipsADirectoryThatFails)
{
    const ScratchDirectory scratch("tileset.");

    saveTileset(scratch.path() / "good", inkedTileset());
    std::filesystem::create_directories(scratch.path() / "bad");
    scratch.write("bad/tileset.json", "not a tileset");

    const auto library = loadTilesetLibrary(scratch.path());

    ASSERT_EQ(library.size(), 1U);
    EXPECT_EQ(library[0].name, "rustwall");
}

TEST(TilesetFileTest, ReadLayerBitmap_RejectsAnIncompleteBitmap)
{
    Tileset set;
    static_cast<void>(addSprite(set, 0));

    auto truncated = blankLayerBitmap(1);
    truncated.pixels.pop_back();

    EXPECT_THROW(
        readLayerBitmap(set.layers[0], truncated), TilesetError);
}

TEST(TilesetFileTest, SaveTileset_ThrowsWhenTheTilesetFileFails)
{
    if (!std::filesystem::exists(kFullDevice))
    {
        GTEST_SKIP() << "no " << kFullDevice << " to fill up";
    }

    const ScratchDirectory scratch("tileset.");
    const auto directory = scratch.path() / "set";
    std::filesystem::create_directories(directory);
    std::filesystem::create_symlink(
        kFullDevice, directory / "tileset.json");

    EXPECT_THROW(
        saveTileset(directory, inkedTileset()), TilesetError);
}

TEST(TilesetFileTest, SaveTileset_ThrowsWhenALayerImageFails)
{
    if (!std::filesystem::exists(kFullDevice))
    {
        GTEST_SKIP() << "no " << kFullDevice << " to fill up";
    }

    const ScratchDirectory scratch("tileset.");
    const auto directory = scratch.path() / "set";
    std::filesystem::create_directories(directory);
    std::filesystem::create_symlink(
        kFullDevice, directory / "layer-0.png");

    EXPECT_THROW(
        saveTileset(directory, inkedTileset()), TilesetError);
}
