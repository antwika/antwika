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
    Bitmap twoByTwo()
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
            .origin = {.x = 10, .y = 20},
            .size = {.width = 30, .height = 40}},
        Color{.green = 255});
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

    const auto texture = renderer.createTexture(twoByTwo());

    ASSERT_NE(texture, nullptr);
    EXPECT_EQ(texture->size(), (Size{.width = 2, .height = 2}));
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

    const auto texture = renderer.createTexture(twoByTwo());
    const Rect whole{
        .origin = {.x = 0, .y = 0},
        .size = {.width = 2, .height = 2}};

    renderer.drawTexture(*texture, whole, whole, Color{.red = 255});
}

TEST(NullRendererTest, DrawTexture_AcceptsATextureItDidNotCreate)
{
    MockLogger logger;
    NullRenderer renderer(logger);
    const NiceMock<MockTexture> foreign;

    EXPECT_CALL(logger, log(Level::Trace, "gfx.null: draw texture"));

    const Rect whole{
        .origin = {.x = 0, .y = 0},
        .size = {.width = 2, .height = 2}};

    EXPECT_NO_THROW(
        renderer.drawTexture(foreign, whole, whole, Color{.red = 255}));
}

TEST(NullRendererTest, Present_DiscardsTheFrameAndTracesIt)
{
    MockLogger logger;
    NullRenderer renderer(logger);

    EXPECT_CALL(logger, log(Level::Trace, "gfx.null: present"));

    renderer.present();
}
