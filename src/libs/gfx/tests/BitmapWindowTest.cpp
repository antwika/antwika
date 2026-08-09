#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/log/Level.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/gfx/BitmapWindow.hpp"
#include "antwika/gfx/Color.hpp"
#include "antwika/gfx/Rect.hpp"
#include "antwika/gfx/Size.hpp"
#include "antwika/gfx/WindowDesc.hpp"
#include "antwika/gfx/WindowId.hpp"

namespace
{
    using antwika::gfx::BitmapWindow;
    using antwika::gfx::Color;
    using antwika::gfx::Rect;
    using antwika::gfx::Size;
    using antwika::gfx::WindowDesc;
    using antwika::gfx::WindowId;
    using antwika::log::Level;
    using antwika::log::mocks::MockLogger;
    using ::testing::NiceMock;

    constexpr Size kSize{.width = 8, .height = 4};

    [[nodiscard]] WindowDesc desc()
    {
        return WindowDesc{.title = "Antwika", .size = kSize};
    }

    class BitmapWindowTest : public ::testing::Test
    {
    protected:
        NiceMock<MockLogger> logger;
        BitmapWindow window{logger, WindowId{7}, desc()};
    };
}

TEST_F(BitmapWindowTest, Constructor_TakesOnWhatItWasAskedFor)
{
    EXPECT_EQ(window.id(), WindowId{7});
    EXPECT_EQ(window.title(), "Antwika");
    EXPECT_EQ(window.size(), kSize);
    EXPECT_EQ(window.configuredSize(), kSize);
    EXPECT_TRUE(window.isOpen());
    EXPECT_FALSE(window.isFullscreen());
    EXPECT_EQ(window.page().size, kSize);
}

TEST_F(BitmapWindowTest, Constructor_OpensFullscreenWhenAskedTo)
{
    WindowDesc filling = desc();
    filling.fullscreen = true;

    const BitmapWindow other(logger, WindowId{8}, filling);

    EXPECT_TRUE(other.isFullscreen());
}

TEST_F(BitmapWindowTest, SetTitle_TakesTheNewName)
{
    window.setTitle("Antwika again");

    EXPECT_EQ(window.title(), "Antwika again");
}

TEST_F(BitmapWindowTest, SetFullscreen_TogglesEitherWay)
{
    window.setFullscreen(true);
    EXPECT_TRUE(window.isFullscreen());

    window.setFullscreen(false);
    EXPECT_FALSE(window.isFullscreen());
}

TEST_F(BitmapWindowTest, Renderer_DrawsOntoThePage)
{
    window.renderer().clear(Color{.red = 10, .green = 20, .blue = 30});

    EXPECT_EQ(window.page().pixels[0], 10);
    EXPECT_EQ(window.page().pixels[1], 20);
    EXPECT_EQ(window.page().pixels[2], 30);
}

TEST_F(BitmapWindowTest, Close_SaysSoOnceAndStaysClosed)
{
    MockLogger talkative;
    BitmapWindow other(talkative, WindowId{9}, desc());

    EXPECT_CALL(talkative, log(Level::Debug, "gfx.bitmap: closed window"));

    other.close();
    EXPECT_FALSE(other.isOpen());

    other.close();
    EXPECT_FALSE(other.isOpen());
}
