#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <memory>

#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/text/GlyphCells.hpp>
#include <antwika/text/GlyphAtlasTextures.hpp>
#include <antwika/gfx/Glyphs.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/gfx/mocks/MockTexture.hpp>
#include <antwika/text/GlyphCellsCache.hpp>

using antwika::gfx::Bitmap;
using antwika::gfx::Color;
using antwika::text::GlyphCellsCache;
using antwika::text::GlyphAtlasTextures;
using antwika::gfx::kGlyphCount;
using antwika::gfx::ITexture;
using antwika::gfx::Point;
using antwika::gfx::Rect;
using antwika::gfx::Size;
using antwika::gfx::TextScale;
using antwika::gfx::mocks::MockRenderer;
using antwika::gfx::mocks::MockTexture;
using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

namespace
{
    constexpr Point kOriginPoint{.x = 10, .y = 20};

    constexpr TextScale kScale{.multiplier = 2};

    constexpr Color kInkColor{
        .red = 200, .green = 100, .blue = 50, .alpha = 255};

    [[nodiscard]] std::unique_ptr<ITexture> getATexture()
    {
        return std::make_unique<NiceMock<MockTexture>>();
    }
}

TEST(GlyphAtlasTexturesTest, Draw_BlitsOneGlyphRatherThanOnePixel)
{
    NiceMock<MockRenderer> renderer;
    GlyphAtlasTextures atlases;

    EXPECT_CALL(renderer, createTexture(_))
        .WillOnce([](const Bitmap &) { return getATexture(); });

    EXPECT_CALL(renderer, drawTexture(_, _, _, kInkColor)).Times(3);

    atlases.draw(renderer, kOriginPoint, "abc", kScale, kInkColor);
}

TEST(GlyphAtlasTexturesTest, Draw_UploadsOneAtlasForEveryLineAtAScale)
{
    NiceMock<MockRenderer> renderer;
    GlyphAtlasTextures atlases;

    EXPECT_CALL(renderer, createTexture(_))
        .Times(1)
        .WillOnce([](const Bitmap &) { return getATexture(); });

    atlases.draw(renderer, kOriginPoint, "one", kScale, kInkColor);
    atlases.draw(renderer, kOriginPoint, "two", kScale, kInkColor);
    atlases.draw(renderer, kOriginPoint, "three", kScale, kInkColor);
}

TEST(GlyphAtlasTexturesTest, Draw_UploadsAnAtlasOfItsOwnForEachScale)
{
    NiceMock<MockRenderer> renderer;
    GlyphAtlasTextures atlases;

    EXPECT_CALL(renderer, createTexture(_))
        .Times(2)
        .WillRepeatedly([](const Bitmap &) { return getATexture(); });

    atlases.draw(renderer, kOriginPoint, "small", TextScale{.multiplier = 1}, kInkColor);
    atlases.draw(renderer, kOriginPoint, "large", TextScale{.multiplier = 3}, kInkColor);
}

TEST(GlyphAtlasTexturesTest, Draw_UploadsAnAtlasCutToTheScaleItWasAsked)
{
    NiceMock<MockRenderer> renderer;
    GlyphAtlasTextures atlases;

    GlyphCellsCache cells;
    const Size cellSize = cells.at(TextScale{.multiplier = 3}).getCellSize();

    Size uploadedSize{};

    EXPECT_CALL(renderer, createTexture(_))
        .WillOnce([&uploadedSize](const Bitmap &atlas) {
            uploadedSize = atlas.size;
            return getATexture();
        });

    atlases.draw(renderer, kOriginPoint, "a", TextScale{.multiplier = 3}, kInkColor);

    EXPECT_EQ(
        uploadedSize,
        (Size{
            .width = static_cast<std::uint32_t>(kGlyphCount) * cellSize.width,
            .height = cellSize.height}));
}

TEST(GlyphAtlasTexturesTest, Draw_TakesNoAtlasForAnEmptyLine)
{
    NiceMock<MockRenderer> renderer;
    GlyphAtlasTextures atlases;

    EXPECT_CALL(renderer, createTexture(_)).Times(0);
    EXPECT_CALL(renderer, drawTexture(_, _, _, _)).Times(0);

    atlases.draw(renderer, kOriginPoint, "", kScale, kInkColor);
}

TEST(GlyphAtlasTexturesTest, Draw_TakesNoAtlasForAScaleOfNothing)
{
    NiceMock<MockRenderer> renderer;
    GlyphAtlasTextures atlases;

    EXPECT_CALL(renderer, createTexture(_)).Times(0);
    EXPECT_CALL(renderer, drawTexture(_, _, _, _)).Times(0);

    atlases.draw(renderer, kOriginPoint, "text", TextScale{.multiplier = 0}, kInkColor);
}

TEST(GlyphAtlasTexturesTest, Draw_DrawsNothingWhenTheAtlasCouldNotBeMade)
{
    NiceMock<MockRenderer> renderer;
    GlyphAtlasTextures atlases;

    EXPECT_CALL(renderer, createTexture(_))
        .WillOnce([](const Bitmap &) { return nullptr; });

    EXPECT_CALL(renderer, drawTexture(_, _, _, _)).Times(0);

    atlases.draw(renderer, kOriginPoint, "abc", kScale, kInkColor);
}

TEST(GlyphAtlasTexturesTest, Draw_BlitsTheAtlasItUploaded)
{
    NiceMock<MockRenderer> renderer;
    GlyphAtlasTextures atlases;

    auto ownedTexture = std::make_unique<NiceMock<MockTexture>>();
    const ITexture &uploadedTexture = *ownedTexture;

    EXPECT_CALL(renderer, createTexture(_))
        .WillOnce([&ownedTexture](const Bitmap &)
                  { return std::move(ownedTexture); });

    EXPECT_CALL(
        renderer, drawTexture(
            ::testing::Ref(uploadedTexture),
            _,
            _,
            kInkColor));

    atlases.draw(renderer, kOriginPoint, "a", kScale, kInkColor);
}
