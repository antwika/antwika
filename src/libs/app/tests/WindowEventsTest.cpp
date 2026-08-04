#include "antwika/app/WindowEvents.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <optional>

#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/WindowEvent.hpp>
#include <antwika/gfx/WindowId.hpp>
#include <antwika/gfx/mocks/MockGfxBackend.hpp>

using antwika::app::closeRequestedOn;
using antwika::gfx::CloseRequested;
using antwika::gfx::Resized;
using antwika::gfx::Size;
using antwika::gfx::WindowEvent;
using antwika::gfx::WindowId;
using antwika::gfx::mocks::MockGfxBackend;
using ::testing::NiceMock;
using ::testing::Return;

namespace
{
    constexpr WindowId kOurWindow{1};
    constexpr WindowId kSomeoneElsesWindow{99};

    [[nodiscard]] WindowEvent closeOf(const WindowId window)
    {
        return WindowEvent{
            .window = window, .payload = CloseRequested{}};
    }

    [[nodiscard]] WindowEvent resizeOf(const WindowId window)
    {
        return WindowEvent{
            .window = window,
            .payload =
                Resized{.size = Size{.width = 10, .height = 10}}};
    }
} // namespace

TEST(WindowEventsTest, AnEmptyQueueAsksForNoClose)
{
    NiceMock<MockGfxBackend> backend;

    EXPECT_CALL(backend, pollEvent()).WillOnce(Return(std::nullopt));

    EXPECT_FALSE(closeRequestedOn(backend, kOurWindow));
}

TEST(WindowEventsTest, ACloseForThisWindowIsReported)
{
    NiceMock<MockGfxBackend> backend;

    EXPECT_CALL(backend, pollEvent())
        .WillOnce(Return(closeOf(kOurWindow)))
        .WillOnce(Return(std::nullopt));

    EXPECT_TRUE(closeRequestedOn(backend, kOurWindow));
}

TEST(WindowEventsTest, ACloseForAnotherWindowIsNotOurs)
{
    NiceMock<MockGfxBackend> backend;

    EXPECT_CALL(backend, pollEvent())
        .WillOnce(Return(closeOf(kSomeoneElsesWindow)))
        .WillOnce(Return(std::nullopt));

    EXPECT_FALSE(closeRequestedOn(backend, kOurWindow));
}

// Something else about our own window says nothing about closing.
// This is the arm a queue holding only closes never takes.
TEST(WindowEventsTest, AnotherEventForThisWindowAsksForNoClose)
{
    NiceMock<MockGfxBackend> backend;

    EXPECT_CALL(backend, pollEvent())
        .WillOnce(Return(resizeOf(kOurWindow)))
        .WillOnce(Return(std::nullopt));

    EXPECT_FALSE(closeRequestedOn(backend, kOurWindow));
}

// The queue is drained whether or not this window is named.
// Leaving another window's events would grow it without bound.
TEST(WindowEventsTest, TheQueueIsDrainedPastEventsThatAreNotOurs)
{
    NiceMock<MockGfxBackend> backend;

    EXPECT_CALL(backend, pollEvent())
        .WillOnce(Return(closeOf(kSomeoneElsesWindow)))
        .WillOnce(Return(resizeOf(kOurWindow)))
        .WillOnce(Return(closeOf(kOurWindow)))
        .WillOnce(Return(std::nullopt));

    EXPECT_TRUE(closeRequestedOn(backend, kOurWindow));
}
