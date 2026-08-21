#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/log/Level.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/gfx/WindowSpec.hpp"

#include "NullWindow.hpp"

using antwika::gfx::WindowSpec;
using antwika::gfx::WindowId;
using antwika::gfx::detail::NullWindow;
using antwika::log::Level;
using antwika::log::mocks::MockLogger;
using ::testing::NiceMock;

namespace
{
    constexpr WindowId kWindowId{7};

    WindowSpec demoSpec()
    {
        return WindowSpec{
            .title = "Antwika",
            .size = {.width = 640, .height = 480}};
    }

    WindowSpec resizableSpec()
    {
        return WindowSpec{
            .title = "Antwika",
            .size = {.width = 640, .height = 480},
            .resizable = true};
    }
}

TEST(NullWindowTest, IsOpen_IsTrueForANewWindow)
{
    NiceMock<MockLogger> logger;
    NullWindow window(logger, kWindowId, demoSpec());

    EXPECT_TRUE(window.isOpen());
}

TEST(NullWindowTest, Id_IsTheIdTheBackendAssigned)
{
    NiceMock<MockLogger> logger;
    NullWindow window(logger, kWindowId, demoSpec());

    EXPECT_EQ(window.id(), kWindowId);
}

TEST(NullWindowTest, Title_IsTheRequestedTitle)
{
    NiceMock<MockLogger> logger;
    NullWindow window(logger, kWindowId, demoSpec());

    EXPECT_EQ(window.title(), "Antwika");
}

TEST(NullWindowTest, Size_IsTheRequestedSize)
{
    NiceMock<MockLogger> logger;
    NullWindow window(logger, kWindowId, demoSpec());

    EXPECT_EQ(window.size().width, 640u);
    EXPECT_EQ(window.size().height, 480u);
}

TEST(NullWindowTest, ConfiguredSize_IsTheRequestedSize)
{
    NiceMock<MockLogger> logger;
    NullWindow window(logger, kWindowId, demoSpec());

    EXPECT_EQ(window.configuredSize(), demoSpec().size);
}

TEST(NullWindowTest, Size_MatchesConfiguredSizeWhenResizable)
{
    NiceMock<MockLogger> logger;
    NullWindow window(logger, kWindowId, resizableSpec());

    EXPECT_EQ(window.size(), resizableSpec().size);
    EXPECT_EQ(window.configuredSize(), resizableSpec().size);
}

TEST(NullWindowTest, ConfiguredSize_SurvivesClosingTheWindow)
{
    NiceMock<MockLogger> logger;
    NullWindow window(logger, kWindowId, demoSpec());

    window.close();

    EXPECT_EQ(window.configuredSize(), demoSpec().size);
    EXPECT_EQ(window.size(), demoSpec().size);
}

TEST(NullWindowTest, SetTitle_ReplacesTheTitle)
{
    NiceMock<MockLogger> logger;
    NullWindow window(logger, kWindowId, demoSpec());

    window.setTitle("Antwika Life");

    EXPECT_EQ(window.title(), "Antwika Life");
}

TEST(NullWindowTest, Renderer_ReturnsTheSameRendererEveryCall)
{
    NiceMock<MockLogger> logger;
    NullWindow window(logger, kWindowId, demoSpec());

    EXPECT_EQ(&window.renderer(), &window.renderer());
}

TEST(NullWindowTest, Close_MakesTheWindowClosed)
{
    NiceMock<MockLogger> logger;
    NullWindow window(logger, kWindowId, demoSpec());

    window.close();

    EXPECT_FALSE(window.isOpen());
}

TEST(NullWindowTest, Close_LogsThatTheWindowWasClosed)
{
    MockLogger logger;
    NullWindow window(logger, kWindowId, demoSpec());

    EXPECT_CALL(logger, log(Level::Debug, "gfx.null: closed window"));

    window.close();
}

TEST(NullWindowTest, Close_IsIdempotentAndOnlyLogsOnce)
{
    MockLogger logger;
    NullWindow window(logger, kWindowId, demoSpec());

    EXPECT_CALL(logger, log(Level::Debug, "gfx.null: closed window")).Times(1);

    window.close();
    window.close();

    EXPECT_FALSE(window.isOpen());
}

TEST(NullWindowTest, IsFullscreen_IsFalseByDefault)
{
    NiceMock<MockLogger> logger;
    NullWindow window(logger, kWindowId, demoSpec());

    EXPECT_FALSE(window.isFullscreen());
}

TEST(NullWindowTest, IsFullscreen_IsWhatTheDescriptionAskedFor)
{
    NiceMock<MockLogger> logger;
    NullWindow window(
        logger,
        kWindowId,
        WindowSpec{
            .title = "Antwika",
            .size = {.width = 640, .height = 480},
            .fullscreen = true});

    EXPECT_TRUE(window.isFullscreen());
}

TEST(NullWindowTest, SetFullscreen_ChangesNeitherSize)
{
    NiceMock<MockLogger> logger;
    NullWindow window(logger, kWindowId, demoSpec());

    window.setFullscreen(true);

    EXPECT_TRUE(window.isFullscreen());
    EXPECT_EQ(window.size(), demoSpec().size);
    EXPECT_EQ(window.configuredSize(), demoSpec().size);

    window.setFullscreen(false);

    EXPECT_FALSE(window.isFullscreen());
}
