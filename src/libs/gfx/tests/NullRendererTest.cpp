#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

#include <antwika/gfx/mocks/MockTexture.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/gfx/Bitmap.hpp"
#include "antwika/gfx/Color.hpp"
#include "antwika/gfx/GfxError.hpp"
#include "antwika/gfx/Point.hpp"
#include "antwika/gfx/Rect.hpp"
#include "antwika/gfx/RectF.hpp"
#include "antwika/gfx/Size.hpp"

#include "NullRenderer.hpp"

using antwika::gfx::Bitmap;
using antwika::gfx::Color;
using antwika::gfx::GfxError;
using antwika::gfx::kBytesPerPixel;
using antwika::gfx::Point;
using antwika::gfx::Rect;
using antwika::gfx::Size;
using antwika::gfx::detail::NullRenderer;
using antwika::gfx::mocks::MockTexture;
using antwika::log::Level;
using antwika::log::mocks::MockLogger;
using ::testing::NiceMock;

namespace
{
    Bitmap getTwoByTwo()
    {
        return Bitmap{
            .size = {.width = 2, .height = 2},
            .pixels = std::vector<std::uint8_t>(4 * kBytesPerPixel, 0)};
    }
}

TEST(NullRendererTest, Clear_DiscardsTheFillAndTracesIt)
{
    MockLogger logger;
    NullRenderer renderer(logger);

    EXPECT_CALL(logger, log(Level::Trace, "gfx.null: clear"));

    renderer.clear(Color{.red = 255});
}

TEST(NullRendererTest, DrawRect_DiscardsTheRectangleAndTracesIt)
{
    MockLogger logger;
    NullRenderer renderer(logger);

    EXPECT_CALL(logger, log(Level::Trace, "gfx.null: draw rect"));

    renderer.drawRect(
        Rect{
            .originPoint = {.x = 10, .y = 20},
            .size = {.width = 30, .height = 40}},
        Color{.green = 255});
}

TEST(NullRendererTest, BeginClip_DiscardsTheAreaAndTracesIt)
{
    MockLogger logger;
    NullRenderer renderer(logger);

    EXPECT_CALL(logger, log(Level::Trace, "gfx.null: begin clip"));

    renderer.beginClip(
        antwika::gfx::RectF{
            Rect{
                .originPoint = {.x = 10, .y = 20},
                .size = {.width = 30, .height = 40}}});
}

TEST(NullRendererTest, EndClip_TracesIt)
{
    MockLogger logger;
    NullRenderer renderer(logger);

    EXPECT_CALL(logger, log(Level::Trace, "gfx.null: end clip"));

    renderer.endClip();
}

TEST(NullRendererTest, DrawLine_DiscardsTheLineAndTracesIt)
{
    MockLogger logger;
    NullRenderer renderer(logger);

    EXPECT_CALL(logger, log(Level::Trace, "gfx.null: draw line"));

    renderer.drawLine(
        Point{.x = 1, .y = 2}, Point{.x = 30, .y = 40}, Color{.red = 255});
}

TEST(NullRendererTest, DrawText_DiscardsTheTextAndTracesIt)
{
    MockLogger logger;
    NullRenderer renderer(logger);

    EXPECT_CALL(logger, log(Level::Trace, "gfx.null: draw text"));

    renderer.drawText(
        Point{.x = 5, .y = 6}, "As", 2, Color{.blue = 255});
}

TEST(NullRendererTest, CreateTexture_ReportsTheBitmapSizeAndTracesIt)
{
    MockLogger logger;
    NullRenderer renderer(logger);

    EXPECT_CALL(logger, log(Level::Trace, "gfx.null: create texture"));

    const auto texture = renderer.createTexture(getTwoByTwo());

    ASSERT_NE(texture, nullptr);
    EXPECT_EQ(texture->getSize(), (Size{.width = 2, .height = 2}));
}

TEST(NullRendererTest, CreateTexture_ThrowsOnAnIncompleteBitmap)
{
    MockLogger logger;
    NullRenderer renderer(logger);

    EXPECT_THROW(
        {
            const auto texture = renderer.createTexture(Bitmap{});
        },
        GfxError);
}

TEST(NullRendererTest, DrawTexture_DiscardsTheBlitAndTracesIt)
{
    MockLogger logger;
    NullRenderer renderer(logger);

    EXPECT_CALL(logger, log(Level::Trace, "gfx.null: create texture"));
    EXPECT_CALL(logger, log(Level::Trace, "gfx.null: draw texture"));

    const auto texture = renderer.createTexture(getTwoByTwo());
    const Rect wholeRect{
        .originPoint = {.x = 0, .y = 0},
        .size = {.width = 2, .height = 2}};

    renderer.drawTexture(*texture, wholeRect, wholeRect, Color{.red = 255});
}

TEST(NullRendererTest, DrawTexture_AcceptsATextureItDidNotCreate)
{
    MockLogger logger;
    NullRenderer renderer(logger);
    const NiceMock<MockTexture> foreignTexture;

    EXPECT_CALL(logger, log(Level::Trace, "gfx.null: draw texture"));

    const Rect wholeRect{
        .originPoint = {.x = 0, .y = 0},
        .size = {.width = 2, .height = 2}};

    EXPECT_NO_THROW(
        renderer.drawTexture(
            foreignTexture,
            wholeRect,
            wholeRect,
            Color{.red = 255}));
}

TEST(NullRendererTest, Present_DiscardsTheFrameAndTracesIt)
{
    MockLogger logger;
    NullRenderer renderer(logger);

    EXPECT_CALL(logger, log(Level::Trace, "gfx.null: present"));

    renderer.present();
}

TEST(NullRendererTest, PushTransform_TracesTheMatrixItWasGiven)
{
    NiceMock<MockLogger> logger;
    NullRenderer renderer(logger);

    EXPECT_CALL(logger, log(Level::Trace, "gfx.null: push transform"));

    renderer.pushTransform(antwika::gfx::getIdentityMatrix());
}

TEST(NullRendererTest, PopTransform_ThrowsWhenNothingIsPushed)
{
    NiceMock<MockLogger> logger;
    NullRenderer renderer(logger);

    EXPECT_THROW(renderer.popTransform(), antwika::gfx::GfxError);
}

TEST(NullRendererTest, PopTransform_UndoesOnePushEach)
{
    NiceMock<MockLogger> logger;
    NullRenderer renderer(logger);

    renderer.pushTransform(antwika::gfx::getIdentityMatrix());
    renderer.pushTransform(antwika::gfx::getIdentityMatrix());
    renderer.popTransform();
    renderer.popTransform();

    EXPECT_THROW(renderer.popTransform(), antwika::gfx::GfxError);
}
