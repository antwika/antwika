#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <memory>

#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/GlyphCells.hpp>
#include <antwika/gfx/GlyphSheetTextures.hpp>
#include <antwika/gfx/Glyphs.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/gfx/mocks/MockTexture.hpp>

using antwika::gfx::Bitmap;
using antwika::gfx::Color;
using antwika::gfx::GlyphCellsCache;
using antwika::gfx::GlyphSheetTextures;
using antwika::gfx::kGlyphCount;
using antwika::gfx::ITexture;
using antwika::gfx::Point;
using antwika::gfx::Rect;
using antwika::gfx::Size;
using antwika::gfx::mocks::MockRenderer;
using antwika::gfx::mocks::MockTexture;
using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

namespace
{
    constexpr Point kOrigin{.x = 10, .y = 20};

    constexpr std::uint32_t kScale = 2;

    constexpr Color kInk{
        .red = 200, .green = 100, .blue = 50, .alpha = 255};

    [[nodiscard]] std::unique_ptr<ITexture> aTexture()
    {
        return std::make_unique<NiceMock<MockTexture>>();
    }
}

TEST(GlyphSheetTexturesTest, Draw_BlitsOneGlyphRatherThanOnePixel)
{
    NiceMock<MockRenderer> renderer;
    GlyphSheetTextures sheets;

    EXPECT_CALL(renderer, createTexture(_))
        .WillOnce([](const Bitmap &) { return aTexture(); });

    EXPECT_CALL(renderer, drawTexture(_, _, _, kInk)).Times(3);

    sheets.draw(renderer, kOrigin, "abc", kScale, kInk);
}

TEST(GlyphSheetTexturesTest, Draw_UploadsOneSheetForEveryLineAtAScale)
{
    NiceMock<MockRenderer> renderer;
    GlyphSheetTextures sheets;

    EXPECT_CALL(renderer, createTexture(_))
        .Times(1)
        .WillOnce([](const Bitmap &) { return aTexture(); });

    sheets.draw(renderer, kOrigin, "one", kScale, kInk);
    sheets.draw(renderer, kOrigin, "two", kScale, kInk);
    sheets.draw(renderer, kOrigin, "three", kScale, kInk);
}

TEST(GlyphSheetTexturesTest, Draw_UploadsASheetOfItsOwnForEachScale)
{
    NiceMock<MockRenderer> renderer;
    GlyphSheetTextures sheets;

    EXPECT_CALL(renderer, createTexture(_))
        .Times(2)
        .WillRepeatedly([](const Bitmap &) { return aTexture(); });

    sheets.draw(renderer, kOrigin, "small", 1, kInk);
    sheets.draw(renderer, kOrigin, "large", 3, kInk);
}

TEST(GlyphSheetTexturesTest, Draw_UploadsASheetCutToTheScaleItWasAsked)
{
    NiceMock<MockRenderer> renderer;
    GlyphSheetTextures sheets;

    GlyphCellsCache cells;
    const Size cell = cells.at(3).cellSize();

    Size uploaded{};

    EXPECT_CALL(renderer, createTexture(_))
        .WillOnce([&uploaded](const Bitmap &sheet) {
            uploaded = sheet.size;
            return aTexture();
        });

    sheets.draw(renderer, kOrigin, "a", 3, kInk);

    EXPECT_EQ(
        uploaded,
        (Size{
            .width = static_cast<std::uint32_t>(kGlyphCount) * cell.width,
            .height = cell.height}));
}

TEST(GlyphSheetTexturesTest, Draw_TakesNoSheetForAnEmptyLine)
{
    NiceMock<MockRenderer> renderer;
    GlyphSheetTextures sheets;

    EXPECT_CALL(renderer, createTexture(_)).Times(0);
    EXPECT_CALL(renderer, drawTexture(_, _, _, _)).Times(0);

    sheets.draw(renderer, kOrigin, "", kScale, kInk);
}

TEST(GlyphSheetTexturesTest, Draw_TakesNoSheetForAScaleOfNothing)
{
    NiceMock<MockRenderer> renderer;
    GlyphSheetTextures sheets;

    EXPECT_CALL(renderer, createTexture(_)).Times(0);
    EXPECT_CALL(renderer, drawTexture(_, _, _, _)).Times(0);

    sheets.draw(renderer, kOrigin, "text", 0, kInk);
}

TEST(GlyphSheetTexturesTest, Draw_DrawsNothingWhenTheSheetCouldNotBeMade)
{
    NiceMock<MockRenderer> renderer;
    GlyphSheetTextures sheets;

    EXPECT_CALL(renderer, createTexture(_))
        .WillOnce([](const Bitmap &) { return nullptr; });

    EXPECT_CALL(renderer, drawTexture(_, _, _, _)).Times(0);

    sheets.draw(renderer, kOrigin, "abc", kScale, kInk);
}

TEST(GlyphSheetTexturesTest, Draw_BlitsTheSheetItUploaded)
{
    NiceMock<MockRenderer> renderer;
    GlyphSheetTextures sheets;

    auto owned = std::make_unique<NiceMock<MockTexture>>();
    const ITexture &uploaded = *owned;

    EXPECT_CALL(renderer, createTexture(_))
        .WillOnce([&owned](const Bitmap &) { return std::move(owned); });

    EXPECT_CALL(
        renderer, drawTexture(::testing::Ref(uploaded), _, _, kInk));

    sheets.draw(renderer, kOrigin, "a", kScale, kInk);
}
