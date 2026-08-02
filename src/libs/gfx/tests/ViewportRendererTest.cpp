#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <memory>

#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/gfx/mocks/MockTexture.hpp>

#include "antwika/gfx/Bitmap.hpp"
#include "antwika/gfx/Color.hpp"
#include "antwika/gfx/Point.hpp"
#include "antwika/gfx/Rect.hpp"
#include "antwika/gfx/Size.hpp"
#include "antwika/gfx/Viewport.hpp"
#include "antwika/gfx/ViewportRenderer.hpp"

using antwika::gfx::Bitmap;
using antwika::gfx::Color;
using antwika::gfx::ITexture;
using antwika::gfx::Point;
using antwika::gfx::Rect;
using antwika::gfx::Size;
using antwika::gfx::Viewport;
using antwika::gfx::ViewportRenderer;
using antwika::gfx::mocks::MockRenderer;
using antwika::gfx::mocks::MockTexture;
using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

namespace
{
    constexpr Size kCanvas{.width = 100, .height = 50};

    // Twice the canvas's height, and wider than twice its width.
    // So the scale is two and there is a bar down each side.
    constexpr Size kWindow{.width = 300, .height = 100};

    constexpr Color kInk{.red = 1, .green = 2, .blue = 3};
} // namespace

TEST(ViewportRendererTest, Viewport_IsTheOneViewportForBothSizes)
{
    NiceMock<MockRenderer> inner;
    const ViewportRenderer view(inner, kWindow, kCanvas);

    EXPECT_EQ(view.viewport(), antwika::gfx::viewportFor(kWindow, kCanvas));
}

// Not transformed, deliberately: a frame starts from the whole area.
// Scaling it would leave the last frame showing in the bars.
TEST(ViewportRendererTest, Clear_FillsTheWholeDrawableArea)
{
    MockRenderer inner;
    ViewportRenderer view(inner, kWindow, kCanvas);

    EXPECT_CALL(inner, clear(kInk));

    view.clear(kInk);
}

TEST(ViewportRendererTest, DrawRect_ScalesAndOffsetsTheRectangle)
{
    MockRenderer inner;
    ViewportRenderer view(inner, kWindow, kCanvas);

    EXPECT_CALL(
        inner,
        drawRect(
            Rect{
                .origin = {.x = 70, .y = 20},
                .size = {.width = 20, .height = 8}},
            kInk));

    view.drawRect(
        Rect{
            .origin = {.x = 10, .y = 10},
            .size = {.width = 10, .height = 4}},
        kInk);
}

TEST(ViewportRendererTest, DrawLine_ScalesAndOffsetsBothEnds)
{
    MockRenderer inner;
    ViewportRenderer view(inner, kWindow, kCanvas);

    EXPECT_CALL(
        inner,
        drawLine(Point{.x = 50, .y = 0}, Point{.x = 250, .y = 100}, kInk));

    view.drawLine(Point{.x = 0, .y = 0}, Point{.x = 100, .y = 50}, kInk);
}

TEST(ViewportRendererTest, DrawText_ScalesTheOriginAndTheGlyphScale)
{
    MockRenderer inner;
    ViewportRenderer view(inner, kWindow, kCanvas);

    EXPECT_CALL(
        inner, drawText(Point{.x = 70, .y = 20}, "hi", std::uint32_t{4}, kInk));

    view.drawText(Point{.x = 10, .y = 10}, "hi", 2, kInk);
}

// A window one and a half times the canvas: the scale is 3/2.
// One whole glyph scale cannot honour that.
// So each glyph is anchored on its own transformed cell instead of drifting.
TEST(ViewportRendererTest, DrawText_AnchorsEachGlyphOnANonWholeScale)
{
    MockRenderer inner;
    ViewportRenderer view(
        inner, Size{.width = 150, .height = 75}, kCanvas);

    EXPECT_CALL(
        inner, drawText(Point{.x = 15, .y = 15}, "h", std::uint32_t{1}, kInk));
    EXPECT_CALL(
        inner, drawText(Point{.x = 24, .y = 15}, "i", std::uint32_t{1}, kInk));

    view.drawText(Point{.x = 10, .y = 10}, "hi", 1, kInk);
}

TEST(ViewportRendererTest, DrawTexture_ScalesTheDestinationAndNotTheSource)
{
    MockRenderer inner;
    ViewportRenderer view(inner, kWindow, kCanvas);
    const NiceMock<MockTexture> texture;

    const Rect source{
        .origin = {.x = 4, .y = 4}, .size = {.width = 8, .height = 8}};

    EXPECT_CALL(
        inner,
        drawTexture(
            _,
            source,
            Rect{
                .origin = {.x = 50, .y = 0},
                .size = {.width = 200, .height = 100}},
            kInk));

    view.drawTexture(
        texture,
        source,
        Rect{
            .origin = {.x = 0, .y = 0},
            .size = {.width = 100, .height = 50}},
        kInk);
}

TEST(ViewportRendererTest, CreateTexture_ComesFromTheWrappedRenderer)
{
    MockRenderer inner;
    ViewportRenderer view(inner, kWindow, kCanvas);

    EXPECT_CALL(inner, createTexture(_))
        .WillOnce(
            []([[maybe_unused]] const Bitmap &bitmap)
            { return std::unique_ptr<ITexture>{}; });

    EXPECT_EQ(view.createTexture(Bitmap{}), nullptr);
}

// A 3D draw is aimed by a matrix, so there is nothing here for it.
TEST(ViewportRendererTest, Renderer3d_IsWhateverTheWrappedRendererHas)
{
    NiceMock<MockRenderer> inner;
    ViewportRenderer view(inner, kWindow, kCanvas);

    EXPECT_EQ(view.renderer3d(), inner.renderer3d());
}

TEST(ViewportRendererTest, Present_ReachesTheWrappedRenderer)
{
    MockRenderer inner;
    ViewportRenderer view(inner, kWindow, kCanvas);

    EXPECT_CALL(inner, present());

    view.present();
}

// Two bars, and only two, on a window wider than the canvas's shape.
TEST(ViewportRendererTest, FillSurround_PaintsThePillarboxes)
{
    MockRenderer inner;
    ViewportRenderer view(inner, kWindow, kCanvas);

    EXPECT_CALL(
        inner,
        drawRect(
            Rect{
                .origin = {.x = 0, .y = 0},
                .size = {.width = 50, .height = 100}},
            kInk));
    EXPECT_CALL(
        inner,
        drawRect(
            Rect{
                .origin = {.x = 250, .y = 0},
                .size = {.width = 50, .height = 100}},
            kInk));

    view.fillSurround(kInk);
}

TEST(ViewportRendererTest, FillSurround_PaintsTheLetterboxes)
{
    MockRenderer inner;
    ViewportRenderer view(inner, Size{.width = 100, .height = 80}, kCanvas);

    EXPECT_CALL(
        inner,
        drawRect(
            Rect{
                .origin = {.x = 0, .y = 0},
                .size = {.width = 100, .height = 15}},
            kInk));
    EXPECT_CALL(
        inner,
        drawRect(
            Rect{
                .origin = {.x = 0, .y = 65},
                .size = {.width = 100, .height = 15}},
            kInk));

    view.fillSurround(kInk);
}

// The headless case, which must cost not one drawing call.
TEST(ViewportRendererTest, FillSurround_DrawsNothingWhenTheCanvasFits)
{
    MockRenderer inner;
    ViewportRenderer view(inner, kCanvas, kCanvas);

    EXPECT_CALL(inner, drawRect(_, _)).Times(0);

    view.fillSurround(kInk);
}

// Every call goes through unchanged when the sizes agree.
// Which is what makes a headless run byte for byte what it was.
TEST(ViewportRendererTest, EveryCall_IsUntouchedWhenTheSizesAgree)
{
    MockRenderer inner;
    ViewportRenderer view(inner, kCanvas, kCanvas);

    const Rect rect{
        .origin = {.x = 3, .y = 4}, .size = {.width = 5, .height = 6}};

    EXPECT_CALL(inner, drawRect(rect, kInk));
    EXPECT_CALL(
        inner, drawText(Point{.x = 3, .y = 4}, "x", std::uint32_t{2}, kInk));

    view.drawRect(rect, kInk);
    view.drawText(Point{.x = 3, .y = 4}, "x", 2, kInk);
}
