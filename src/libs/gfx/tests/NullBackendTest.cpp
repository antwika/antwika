// The shared backend contract lives in NullBackendConformanceTest.cpp.
// Only what is specific to this backend is left here.
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/log/Level.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/gfx/NullBackend.hpp"

using antwika::gfx::NullBackend;
using antwika::gfx::rawValue;
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

TEST(NullBackendTest, CreateWindow_ReportsExactlyTheRequestedSize)
{
    NiceMock<MockLogger> logger;
    NullBackend backend(logger);

    const auto window = backend.createWindow(
        WindowDesc{.title = "Antwika", .size = {.width = 640, .height = 480}});

    // Stronger than the conformance suite asks for, and only true here:
    // nothing can resize a window that was never put on a screen.
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

TEST(NullBackendTest, CreateWindow_NumbersWindowsFromOneUpwards)
{
    NiceMock<MockLogger> logger;
    NullBackend backend(logger);

    // Stronger than the conformance suite asks for, and only true here:
    // a real backend inherits whatever ids its framework hands out.
    const auto first = backend.createWindow(WindowDesc{.title = "First"});
    const auto second = backend.createWindow(WindowDesc{.title = "Second"});

    EXPECT_EQ(rawValue(first->id()), 1u);
    EXPECT_EQ(rawValue(second->id()), 2u);
}

TEST(NullBackendTest, PollEvent_AlwaysReturnsNulloptWithNothingToReport)
{
    NiceMock<MockLogger> logger;
    NullBackend backend(logger);

    const auto window = backend.createWindow(WindowDesc{.title = "Antwika"});

    EXPECT_FALSE(backend.pollEvent().has_value());
    EXPECT_FALSE(backend.pollEvent().has_value());
}
