#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/log/Level.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/gfx/GfxError.hpp"
#include "antwika/gfx/NullBackend.hpp"

using antwika::gfx::GfxError;
using antwika::gfx::NullBackend;
using antwika::gfx::WindowDesc;
using antwika::log::Level;
using antwika::log::mocks::MockLogger;
using ::testing::NiceMock;

TEST(NullBackendTest, Name_IsNull)
{
    NiceMock<MockLogger> logger;
    NullBackend backend(logger);

    EXPECT_EQ(backend.name(), "null");
}

TEST(NullBackendTest, CreateWindow_ReturnsAnOpenWindowWithTheRequestedDesc)
{
    NiceMock<MockLogger> logger;
    NullBackend backend(logger);

    const auto window = backend.createWindow(
        WindowDesc{.title = "Antwika", .size = {.width = 640, .height = 480}});

    EXPECT_TRUE(window->isOpen());
    EXPECT_EQ(window->title(), "Antwika");
    EXPECT_EQ(window->size().width, 640u);
    EXPECT_EQ(window->size().height, 480u);
}

TEST(NullBackendTest, CreateWindow_LogsThatAWindowWasCreated)
{
    MockLogger logger;
    NullBackend backend(logger);

    EXPECT_CALL(logger, log(Level::Debug, "gfx.null: created window"));

    const auto window = backend.createWindow(WindowDesc{.title = "Antwika"});
}

TEST(NullBackendTest, CreateWindow_ThrowsWhenWidthIsZero)
{
    NiceMock<MockLogger> logger;
    NullBackend backend(logger);

    EXPECT_THROW(
        {
            const auto window = backend.createWindow(WindowDesc{
                .title = "Antwika",
                .size = {.width = 0, .height = 480}});
        },
        GfxError);
}

TEST(NullBackendTest, CreateWindow_ThrowsWhenHeightIsZero)
{
    NiceMock<MockLogger> logger;
    NullBackend backend(logger);

    EXPECT_THROW(
        {
            const auto window = backend.createWindow(WindowDesc{
                .title = "Antwika",
                .size = {.width = 640, .height = 0}});
        },
        GfxError);
}

TEST(NullBackendTest, CreateWindow_ReturnsIndependentWindows)
{
    NiceMock<MockLogger> logger;
    NullBackend backend(logger);

    const auto first = backend.createWindow(WindowDesc{.title = "First"});
    const auto second = backend.createWindow(WindowDesc{.title = "Second"});

    first->close();

    EXPECT_FALSE(first->isOpen());
    EXPECT_TRUE(second->isOpen());
    EXPECT_EQ(second->title(), "Second");
}

TEST(NullBackendTest, PollEvent_ReturnsNulloptBecauseNothingReportsEvents)
{
    NiceMock<MockLogger> logger;
    NullBackend backend(logger);

    EXPECT_FALSE(backend.pollEvent().has_value());
}
