#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <vector>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/RectF.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/tileset/PixelClass.hpp>
#include <antwika/tileset/Sprite.hpp>
#include <antwika/tileset/Tileset.hpp>

#include "antwika/map_editor/ToolIcons.hpp"

using antwika::gfx::Color;
using antwika::gfx::Rect;
using antwika::gfx::RectF;
using antwika::gfx::mocks::MockRenderer;
using antwika::map_editor::drawIconGlyph;
using antwika::map_editor::IconGlyph;
using antwika::map_editor::kIconGlyphSide;
using antwika::map_editor::kSelectToolGlyph;
using antwika::map_editor::terrainIconBitmap;
using antwika::tileset::addSprite;
using antwika::tileset::PixelClass;
using antwika::tileset::Tileset;
using ::testing::_;
using ::testing::NiceMock;

namespace
{
    constexpr Color kInk{.red = 10, .green = 20, .blue = 30};
    constexpr Color kPaper{.red = 40, .green = 50, .blue = 60};

    [[nodiscard]] Rect boxAt(
        const std::int32_t x,
        const std::int32_t y,
        const std::uint32_t side)
    {
        return Rect{
            .origin = {.x = x, .y = y},
            .size = {.width = side, .height = side}};
    }

    [[nodiscard]] std::size_t bitsIn(const IconGlyph &glyph)
    {
        std::size_t count = 0;

        for (const auto row : glyph)
        {
            for (std::size_t bit = 0; bit < kIconGlyphSide; ++bit)
            {
                count += (row & (0x80U >> bit)) != 0 ? 1 : 0;
            }
        }

        return count;
    }
}

TEST(ToolIconsTest, DrawIconGlyph_DrawsOnePixelPerSetBit)
{
    NiceMock<MockRenderer> view;
    constexpr IconGlyph glyph{
        0b10000001, 0, 0, 0, 0, 0, 0, 0b00000001};

    EXPECT_CALL(view, drawRect(_, _)).Times(3);

    drawIconGlyph(view, boxAt(0, 0, 8), glyph, kInk);
}

TEST(ToolIconsTest, DrawIconGlyph_DrawsNothingForABlankGlyph)
{
    NiceMock<MockRenderer> view;
    constexpr IconGlyph blank{};

    EXPECT_CALL(view, drawRect(_, _)).Times(0);

    drawIconGlyph(view, boxAt(0, 0, 8), blank, kInk);
}

TEST(ToolIconsTest, DrawIconGlyph_CentersTheGlyphInTheBox)
{
    NiceMock<MockRenderer> view;
    constexpr IconGlyph corner{0b10000000, 0, 0, 0, 0, 0, 0, 0};

    EXPECT_CALL(
        view,
        drawRect(RectF({6.0F, 8.0F}, {1.0F, 1.0F}), kInk))
        .Times(1);

    drawIconGlyph(view, boxAt(4, 6, 12), corner, kInk);
}

TEST(ToolIconsTest, DrawIconGlyph_PlacesBitSevenLeftmost)
{
    NiceMock<MockRenderer> view;
    constexpr IconGlyph rightmost{0b00000001, 0, 0, 0, 0, 0, 0, 0};

    EXPECT_CALL(
        view,
        drawRect(RectF({7.0F, 0.0F}, {1.0F, 1.0F}), kInk))
        .Times(1);

    drawIconGlyph(view, boxAt(0, 0, 8), rightmost, kInk);
}

TEST(ToolIconsTest, DrawIconGlyph_DrawsTheDeclaredToolGlyphs)
{
    NiceMock<MockRenderer> view;

    EXPECT_CALL(view, drawRect(_, _))
        .Times(static_cast<int>(bitsIn(kSelectToolGlyph)));

    drawIconGlyph(view, boxAt(0, 0, 8), kSelectToolGlyph, kInk);
}

TEST(ToolIconsTest, TerrainIconBitmap_IsOneSpriteSquare)
{
    const auto bitmap = terrainIconBitmap(Tileset{}, kInk, kPaper);

    EXPECT_EQ(bitmap.size.width, 8U);
    EXPECT_EQ(bitmap.size.height, 8U);
    EXPECT_TRUE(bitmap.isComplete());
}

TEST(ToolIconsTest, TerrainIconBitmap_IsClearWithoutABaseSprite)
{
    const auto bitmap = terrainIconBitmap(Tileset{}, kInk, kPaper);

    for (const auto byte : bitmap.pixels)
    {
        EXPECT_EQ(byte, 0);
    }
}

TEST(ToolIconsTest, TerrainIconBitmap_IsClearWithoutAnyLayer)
{
    Tileset set;
    set.layers.clear();

    const auto bitmap = terrainIconBitmap(set, kInk, kPaper);

    for (const auto byte : bitmap.pixels)
    {
        EXPECT_EQ(byte, 0);
    }
}

TEST(ToolIconsTest, TerrainIconBitmap_ColorsInkAndPaperPixels)
{
    Tileset set;
    auto &sprite = addSprite(set, 0);
    sprite.frames[0].pixels[0] = PixelClass::Ink;
    sprite.frames[0].pixels[1] = PixelClass::Paper;

    const auto bitmap = terrainIconBitmap(set, kInk, kPaper);

    EXPECT_EQ(bitmap.pixels[0], kInk.red);
    EXPECT_EQ(bitmap.pixels[1], kInk.green);
    EXPECT_EQ(bitmap.pixels[2], kInk.blue);
    EXPECT_EQ(bitmap.pixels[3], 255);
    EXPECT_EQ(bitmap.pixels[4], kPaper.red);
    EXPECT_EQ(bitmap.pixels[7], 255);
}

TEST(ToolIconsTest, TerrainIconBitmap_LeavesBlankPixelsTransparent)
{
    Tileset set;
    auto &sprite = addSprite(set, 0);
    sprite.frames[0].pixels[0] = PixelClass::Ink;

    const auto bitmap = terrainIconBitmap(set, kInk, kPaper);

    EXPECT_EQ(bitmap.pixels[7], 0);
}
