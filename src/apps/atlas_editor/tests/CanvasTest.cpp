#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/atlas_editor/AtlasEditorError.hpp"
#include "antwika/atlas_editor/Canvas.hpp"
#include "antwika/atlas_editor/Pixel.hpp"

using antwika::atlas_editor::AtlasEditorError;
using antwika::atlas_editor::Canvas;
using antwika::atlas_editor::Pixel;
using antwika::gfx::Bitmap;
using antwika::gfx::Color;
using antwika::gfx::Size;

namespace
{
    constexpr Color kRed{.red = 255, .green = 0, .blue = 0, .alpha = 255};
    constexpr Color kClear{
        .red = 0, .green = 0, .blue = 0, .alpha = 0};

    Canvas twoByTwo()
    {
        return Canvas::blank(Size{.width = 2, .height = 2});
    }
}

TEST(CanvasTest, Blank_HoldsEveryPixelItClaimsTo)
{
    const Canvas canvas = twoByTwo();

    EXPECT_EQ(canvas.size(), (Size{.width = 2, .height = 2}));
    EXPECT_TRUE(canvas.bitmap().isComplete());
    EXPECT_EQ(canvas.bitmap().pixels.size(), 16U);
}

TEST(CanvasTest, Blank_StartsFullyTransparent)
{
    const Canvas canvas = twoByTwo();

    EXPECT_EQ(canvas.at(Pixel{.x = 0, .y = 0}), kClear);
    EXPECT_EQ(canvas.at(Pixel{.x = 1, .y = 1}), kClear);
    EXPECT_EQ(canvas.revision(), 0U);
}

TEST(CanvasTest, Blank_ThrowsOnASheetWithNoExtent)
{
    EXPECT_THROW(
        static_cast<void>(Canvas::blank(Size{.width = 0, .height = 4})),
        AtlasEditorError);
}

TEST(CanvasTest, Ctor_ThrowsOnAnIncompleteBitmap)
{
    const Bitmap wrong{
        .size = {.width = 2, .height = 2},
        .pixels = std::vector<std::uint8_t>(4, 0)};

    EXPECT_THROW(
        static_cast<void>(Canvas{wrong}), AtlasEditorError);
}

TEST(CanvasTest, Ctor_TakesTheBitmapAsItIs)
{
    const Bitmap image{
        .size = {.width = 1, .height = 1},
        .pixels = std::vector<std::uint8_t>{1, 2, 3, 4}};

    const Canvas canvas(image);

    EXPECT_EQ(
        canvas.at(Pixel{.x = 0, .y = 0}),
        (Color{.red = 1, .green = 2, .blue = 3, .alpha = 4}));
}

TEST(CanvasTest, Holds_AnswersForEveryWayOfBeingOutside)
{
    const Canvas canvas = twoByTwo();

    EXPECT_TRUE(canvas.holds(Pixel{.x = 1, .y = 1}));
    EXPECT_FALSE(canvas.holds(Pixel{.x = -1, .y = 0}));
    EXPECT_FALSE(canvas.holds(Pixel{.x = 0, .y = -1}));
    EXPECT_FALSE(canvas.holds(Pixel{.x = 2, .y = 0}));
    EXPECT_FALSE(canvas.holds(Pixel{.x = 0, .y = 2}));
}

TEST(CanvasTest, At_ReadsBeyondTheEdgeAsTransparent)
{
    const Canvas canvas = twoByTwo();

    EXPECT_EQ(canvas.at(Pixel{.x = 9, .y = 9}), kClear);
}

TEST(CanvasTest, Set_WritesAPixelAndCountsTheChange)
{
    Canvas canvas = twoByTwo();

    EXPECT_TRUE(canvas.set(Pixel{.x = 1, .y = 0}, kRed));
    EXPECT_EQ(canvas.at(Pixel{.x = 1, .y = 0}), kRed);
    EXPECT_EQ(canvas.revision(), 1U);

    EXPECT_EQ(canvas.at(Pixel{.x = 0, .y = 0}), kClear);
}

TEST(CanvasTest, Set_IsNoChangeWhenThePixelAlreadyHoldsThatColour)
{
    Canvas canvas = twoByTwo();

    EXPECT_TRUE(canvas.set(Pixel{.x = 0, .y = 1}, kRed));
    EXPECT_FALSE(canvas.set(Pixel{.x = 0, .y = 1}, kRed));
    EXPECT_EQ(canvas.revision(), 1U);
}

TEST(CanvasTest, Set_IgnoresAPixelOutsideTheSheet)
{
    Canvas canvas = twoByTwo();

    EXPECT_FALSE(canvas.set(Pixel{.x = -1, .y = 0}, kRed));
    EXPECT_EQ(canvas.revision(), 0U);
}

TEST(CanvasTest, Bitmap_IsWhatAWriterWouldEncode)
{
    Canvas canvas = twoByTwo();
    static_cast<void>(canvas.set(Pixel{.x = 0, .y = 0}, kRed));

    EXPECT_EQ(
        canvas.bitmap().pixels,
        (std::vector<std::uint8_t>{
            255, 0, 0, 255, 0, 0, 0, 0,
            0,   0, 0, 0,   0, 0, 0, 0}));
}
