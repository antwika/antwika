#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/log/Level.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/gfx/NullBackend.hpp"

using antwika::gfx::NullBackend;
using antwika::gfx::getRawValue;
using antwika::gfx::WindowSpec;
using antwika::log::Level;
using antwika::log::mocks::MockLogger;
using ::testing::NiceMock;

TEST(NullBackendTest, Name_IsNull)
{
    NiceMock<MockLogger> logger;
    NullBackend backend(logger);

    EXPECT_EQ(backend.getName(), "null");
}

TEST(NullBackendTest, CreateWindow_ReportsExactlyTheRequestedSize)
{
    NiceMock<MockLogger> logger;
    NullBackend backend(logger);

    const auto window = backend.createWindow(
        WindowSpec{.title = "Antwika", .size = {.width = 640, .height = 480}});

    EXPECT_EQ(window->getSize().width, 640u);
    EXPECT_EQ(window->getSize().height, 480u);
}

TEST(NullBackendTest, CreateWindow_LogsThatAWindowWasCreated)
{
    MockLogger logger;
    NullBackend backend(logger);

    EXPECT_CALL(logger, log(Level::Debug, "gfx.null: created window"));

    const auto window = backend.createWindow(WindowSpec{.title = "Antwika"});
}

TEST(NullBackendTest, CreateWindow_NumbersWindowsFromOneUpwards)
{
    NiceMock<MockLogger> logger;
    NullBackend backend(logger);

    const auto first = backend.createWindow(WindowSpec{.title = "First"});
    const auto second = backend.createWindow(WindowSpec{.title = "Second"});

    EXPECT_EQ(getRawValue(first->getId()), 1u);
    EXPECT_EQ(getRawValue(second->getId()), 2u);
}

TEST(NullBackendTest, PollEvent_AlwaysReturnsNulloptWithNothingToReport)
{
    NiceMock<MockLogger> logger;
    NullBackend backend(logger);

    const auto window = backend.createWindow(WindowSpec{.title = "Antwika"});

    EXPECT_FALSE(backend.pollEvent().has_value());
    EXPECT_FALSE(backend.pollEvent().has_value());
}
