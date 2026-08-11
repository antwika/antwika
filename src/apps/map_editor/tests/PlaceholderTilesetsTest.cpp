#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <set>

#include <antwika/enums/Enumeration.hpp>
#include <antwika/gfx/Bitmap.hpp>
#include <antwika/tilemap/TerrainClass.hpp>
#include <antwika/tileset/PixelClass.hpp>
#include <antwika/tileset/Sprite.hpp>
#include <antwika/tileset/Tileset.hpp>

#include "antwika/map_editor/PlaceholderTilesets.hpp"

using antwika::tilemap::TerrainClass;
using antwika::map_editor::placeholderSystemSheet;
using antwika::map_editor::placeholderTileset;
using antwika::tileset::kEdgeSocket;
using antwika::tileset::kSpriteSide;
using antwika::tileset::PixelClass;
using antwika::tileset::Side;

namespace
{
    constexpr std::size_t kNorth = 0;
    constexpr std::size_t kEast = 1;
    constexpr std::size_t kSouth = 2;
    constexpr std::size_t kWest = 3;

    [[nodiscard]] bool isWhite(
        const antwika::gfx::Bitmap &sheet,
        const std::uint32_t x,
        const std::uint32_t y)
    {
        const auto offset =
            (static_cast<std::size_t>(y) * sheet.size.width + x)
            * antwika::gfx::kBytesPerPixel;

        return sheet.pixels[offset] == 255
               && sheet.pixels[offset + 3] == 255;
    }

    [[nodiscard]] PixelClass pixelOf(
        const antwika::tileset::Sprite &sprite,
        const std::int32_t x,
        const std::int32_t y)
    {
        return sprite.frames[0].pixels[static_cast<std::size_t>(
            y * kSpriteSide + x)];
    }
}

TEST(PlaceholderTilesetsTest, PlaceholderTileset_HoldsNineBaseSprites)
{
    for (const auto terrain :
         antwika::enums::kAll<TerrainClass>)
    {
        const auto set = placeholderTileset(terrain);

        ASSERT_EQ(set.layers.size(), 1U);
        EXPECT_EQ(set.layers[0].sprites.size(), 9U);
        EXPECT_EQ(set.terrain, terrain);
    }
}

TEST(PlaceholderTilesetsTest, PlaceholderTileset_NamesItselfAfterTheTerrain)
{
    EXPECT_EQ(
        placeholderTileset(TerrainClass::Water).name,
        "placeholder-water");
}

TEST(PlaceholderTilesetsTest, PlaceholderTileset_FacesEdgeSocketsOutward)
{
    const auto set = placeholderTileset(TerrainClass::Floor);
    const auto &sprites = set.layers[0].sprites;

    EXPECT_NE(sprites[0].sockets[kNorth], kEdgeSocket);
    EXPECT_EQ(sprites[1].sockets[kNorth], kEdgeSocket);
    EXPECT_EQ(sprites[2].sockets[kEast], kEdgeSocket);
    EXPECT_EQ(sprites[3].sockets[kSouth], kEdgeSocket);
    EXPECT_EQ(sprites[4].sockets[kWest], kEdgeSocket);
    EXPECT_EQ(sprites[5].sockets[kNorth], kEdgeSocket);
    EXPECT_EQ(sprites[5].sockets[kWest], kEdgeSocket);
    EXPECT_EQ(sprites[8].sockets[kEast], kEdgeSocket);
    EXPECT_EQ(sprites[8].sockets[kSouth], kEdgeSocket);
}

TEST(PlaceholderTilesetsTest, PlaceholderTileset_InksTheDeclaredBorders)
{
    const auto set = placeholderTileset(TerrainClass::Floor);
    const auto &sprites = set.layers[0].sprites;

    EXPECT_EQ(pixelOf(sprites[1], 4, 0), PixelClass::Ink);
    EXPECT_EQ(pixelOf(sprites[2], kSpriteSide - 1, 4), PixelClass::Ink);
    EXPECT_EQ(pixelOf(sprites[3], 4, kSpriteSide - 1), PixelClass::Ink);
    EXPECT_EQ(pixelOf(sprites[4], 0, 4), PixelClass::Ink);
}

TEST(PlaceholderTilesetsTest, PlaceholderTileset_TellsTheTerrainsApart)
{
    std::set<std::vector<PixelClass>> seen;

    for (const auto terrain : antwika::enums::kAll<TerrainClass>)
    {
        const auto set = placeholderTileset(terrain);
        const auto &pixels = set.layers[0].sprites[0].frames[0].pixels;

        seen.insert(
            std::vector<PixelClass>(pixels.begin(), pixels.end()));
    }

    EXPECT_EQ(seen.size(), antwika::enums::kCount<TerrainClass>);
}

TEST(PlaceholderTilesetsTest, PlaceholderTileset_LeavesUnpatternedCellsBlank)
{
    const auto set = placeholderTileset(TerrainClass::Floor);
    const auto &interior = set.layers[0].sprites[0];

    EXPECT_EQ(pixelOf(interior, 3, 5), PixelClass::Paper);
    EXPECT_EQ(pixelOf(interior, 6, 1), PixelClass::Paper);
    EXPECT_EQ(pixelOf(interior, 0, 0), PixelClass::Blank);
}

TEST(PlaceholderTilesetsTest, PlaceholderSystemSheet_IsThirtyTwoByEight)
{
    const auto sheet = placeholderSystemSheet();

    EXPECT_EQ(sheet.size.width, 32U);
    EXPECT_EQ(sheet.size.height, 8U);
    EXPECT_TRUE(sheet.isComplete());
}

TEST(PlaceholderSystemSheetTest, PlaceholderSystemSheet_InksEachSlot)
{
    const auto sheet = placeholderSystemSheet();

    EXPECT_TRUE(isWhite(sheet, 0, 0));
    EXPECT_TRUE(isWhite(sheet, 8, 0));
    EXPECT_TRUE(isWhite(sheet, 16, 1));
    EXPECT_TRUE(isWhite(sheet, 24, 0));
}

TEST(PlaceholderSystemSheetTest, PlaceholderSystemSheet_LeavesGapsBetweenInk)
{
    const auto sheet = placeholderSystemSheet();

    EXPECT_FALSE(isWhite(sheet, 1, 1));
    EXPECT_FALSE(isWhite(sheet, 8 + 1, 2));
    EXPECT_FALSE(isWhite(sheet, 16 + 0, 0));
    EXPECT_FALSE(isWhite(sheet, 24 + 1, 0));
}

TEST(PlaceholderTilesetsTest, PlaceholderTileset_LeavesAnUnknownTerrainBlank)
{
    const auto set =
        placeholderTileset(static_cast<TerrainClass>(42));
    const auto &interior = set.layers[0].sprites[0];

    for (std::int32_t y = 0; y < kSpriteSide; ++y)
    {
        for (std::int32_t x = 0; x < kSpriteSide; ++x)
        {
            EXPECT_EQ(pixelOf(interior, x, y), PixelClass::Blank);
        }
    }
}
