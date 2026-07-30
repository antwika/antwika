#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/log/Level.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/gfx/Color.hpp"
#include "antwika/gfx/Rect.hpp"

#include "NullRenderer.hpp"

using antwika::gfx::Color;
using antwika::gfx::Rect;
using antwika::gfx::detail::NullRenderer;
using antwika::log::Level;
using antwika::log::mocks::MockLogger;

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

TEST(NullRendererTest, Present_DiscardsTheFrameAndTracesIt)
{
    MockLogger logger;
    NullRenderer renderer(logger);

    EXPECT_CALL(logger, log(Level::Trace, "gfx.null: present"));

    renderer.present();
}
