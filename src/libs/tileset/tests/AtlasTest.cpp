#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <vector>

#include <antwika/geometry/Rect.hpp>
#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/tileset/Atlas.hpp>
#include <antwika/tileset/PixelClass.hpp>
#include <antwika/tileset/Sprite.hpp>
#include <antwika/tileset/Tileset.hpp>

using antwika::geometry::Rect;
using antwika::gfx::Color;
using antwika::tileset::addLayer;
using antwika::tileset::addSprite;
using antwika::tileset::atlasIndexOf;
using antwika::tileset::atlasSource;
using antwika::tileset::bakeAtlas;
using antwika::tileset::kAtlasWidth;
using antwika::tileset::PixelClass;
using antwika::tileset::Tileset;

namespace
{
    constexpr Color kInk{.red = 10, .green = 20, .blue = 30};

    constexpr Color kPaper{.red = 40, .green = 50, .blue = 60};

    [[nodiscard]] Color colorAt(
        const antwika::gfx::Bitmap &bitmap,
        const std::uint32_t x,
        const std::uint32_t y)
    {
        const auto offset = static_cast<std::size_t>(
                                y * bitmap.size.width + x)
                            * antwika::gfx::kBytesPerPixel;

        return Color{
            .red = bitmap.pixels[offset],
            .green = bitmap.pixels[offset + 1],
            .blue = bitmap.pixels[offset + 2],
            .alpha = bitmap.pixels[offset + 3]};
    }
}

TEST(AtlasTest, AtlasIndexOf_StacksLayersInOrderOneRowPerSprite)
{
    Tileset set;
    static_cast<void>(addSprite(set, 0));
    static_cast<void>(addSprite(set, 0));
    static_cast<void>(addLayer(set, "moss"));
    static_cast<void>(addSprite(set, 1));
    static_cast<void>(addSprite(set, 1));
    static_cast<void>(addSprite(set, 1));

    const auto index = atlasIndexOf(set);

    EXPECT_EQ(
        index.layerRowOffsets,
        (std::vector<std::uint32_t>{0, 2}));
    EXPECT_EQ(index.rows, 5U);
}

TEST(AtlasTest, AtlasIndexOf_GivesAnEmptyTilesetNoRows)
{
    const auto index = atlasIndexOf(Tileset{});

    EXPECT_EQ(
        index.layerRowOffsets, (std::vector<std::uint32_t>{0}));
    EXPECT_EQ(index.rows, 0U);
}

TEST(AtlasTest, AtlasSource_PlacesFramesAndRowsEightPixelsApart)
{
    EXPECT_EQ(kAtlasWidth, 32);
    EXPECT_EQ(
        atlasSource(0, 0),
        (Rect{
            .origin = {.x = 0, .y = 0},
            .size = {.width = 8, .height = 8}}));
    EXPECT_EQ(
        atlasSource(3, 2),
        (Rect{
            .origin = {.x = 16, .y = 24},
            .size = {.width = 8, .height = 8}}));
}

TEST(AtlasTest, BakeAtlas_TintsInkAndPaperAndLeavesBlankClear)
{
    Tileset set;

    auto &sprite = addSprite(set, 0);
    sprite.frames[0].pixels[0] = PixelClass::Ink;
    sprite.frames[0].pixels[1] = PixelClass::Paper;

    const auto baked = bakeAtlas(set, kInk, kPaper);

    ASSERT_EQ(baked.size.width, 32U);
    ASSERT_EQ(baked.size.height, 8U);
    ASSERT_TRUE(baked.isComplete());
    EXPECT_EQ(
        colorAt(baked, 0, 0),
        (Color{.red = 10, .green = 20, .blue = 30, .alpha = 255}));
    EXPECT_EQ(
        colorAt(baked, 1, 0),
        (Color{.red = 40, .green = 50, .blue = 60, .alpha = 255}));
    EXPECT_EQ(colorAt(baked, 2, 0).alpha, 0);
}

TEST(AtlasTest, BakeAtlas_LeavesFrameSlotsPastFrameCountClear)
{
    Tileset set;

    auto &sprite = addSprite(set, 0);
    sprite.frames[1].pixels[0] = PixelClass::Ink;

    const auto baked = bakeAtlas(set, kInk, kPaper);

    EXPECT_EQ(colorAt(baked, 8, 0).alpha, 0);
}

TEST(AtlasTest, BakeAtlas_PaintsEveryFrameWithinFrameCount)
{
    Tileset set;

    auto &sprite = addSprite(set, 0);
    sprite.frameCount = 3;
    sprite.frames[1].pixels[0] = PixelClass::Ink;
    sprite.frames[2].pixels[0] = PixelClass::Paper;

    const auto baked = bakeAtlas(set, kInk, kPaper);

    EXPECT_EQ(colorAt(baked, 8, 0).red, 10);
    EXPECT_EQ(colorAt(baked, 16, 0).red, 40);
}

TEST(AtlasTest, BakeAtlas_ClampsAFrameCountPastTheFrameArray)
{
    Tileset set;

    auto &sprite = addSprite(set, 0);
    sprite.frameCount = 9;
    sprite.frames[2].pixels[0] = PixelClass::Ink;

    const auto baked = bakeAtlas(set, kInk, kPaper);

    ASSERT_EQ(baked.size.width, 32U);
    EXPECT_EQ(colorAt(baked, 16, 0).red, 10);
}

TEST(AtlasTest, BakeAtlas_PlacesASecondLayersSpriteBelowTheBase)
{
    Tileset set;
    addSprite(set, 0).frames[0].pixels[0] = PixelClass::Ink;
    static_cast<void>(addLayer(set, "moss"));
    addSprite(set, 1).frames[0].pixels[0] = PixelClass::Paper;

    const auto baked = bakeAtlas(set, kInk, kPaper);

    ASSERT_EQ(baked.size.height, 16U);
    EXPECT_EQ(
        colorAt(baked, 0, 8),
        (Color{.red = 40, .green = 50, .blue = 60, .alpha = 255}));
}
