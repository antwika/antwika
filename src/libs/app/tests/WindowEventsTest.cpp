#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <optional>

#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/WindowEvent.hpp>
#include <antwika/gfx/WindowId.hpp>
#include <antwika/gfx/mocks/MockGfxBackend.hpp>

#include "antwika/app/WindowEvents.hpp"

using antwika::app::closeRequestedOn;
using antwika::app::windowChanges;
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
}

TEST(WindowEventsTest, CloseRequestedOn_IsFalseForAnEmptyQueue)
{
    NiceMock<MockGfxBackend> backend;

    EXPECT_CALL(backend, pollEvent()).WillOnce(Return(std::nullopt));

    EXPECT_FALSE(closeRequestedOn(backend, kOurWindow));
}

TEST(WindowEventsTest, CloseRequestedOn_ReportsThisWindowsClose)
{
    NiceMock<MockGfxBackend> backend;

    EXPECT_CALL(backend, pollEvent())
        .WillOnce(Return(closeOf(kOurWindow)))
        .WillOnce(Return(std::nullopt));

    EXPECT_TRUE(closeRequestedOn(backend, kOurWindow));
}

TEST(WindowEventsTest, CloseRequestedOn_IgnoresAnotherWindow)
{
    NiceMock<MockGfxBackend> backend;

    EXPECT_CALL(backend, pollEvent())
        .WillOnce(Return(closeOf(kSomeoneElsesWindow)))
        .WillOnce(Return(std::nullopt));

    EXPECT_FALSE(closeRequestedOn(backend, kOurWindow));
}

TEST(WindowEventsTest, CloseRequestedOn_IgnoresAnotherEvent)
{
    NiceMock<MockGfxBackend> backend;

    EXPECT_CALL(backend, pollEvent())
        .WillOnce(Return(resizeOf(kOurWindow)))
        .WillOnce(Return(std::nullopt));

    EXPECT_FALSE(closeRequestedOn(backend, kOurWindow));
}

TEST(WindowEventsTest, CloseRequestedOn_DrainsPastOtherEvents)
{
    NiceMock<MockGfxBackend> backend;

    EXPECT_CALL(backend, pollEvent())
        .WillOnce(Return(closeOf(kSomeoneElsesWindow)))
        .WillOnce(Return(resizeOf(kOurWindow)))
        .WillOnce(Return(closeOf(kOurWindow)))
        .WillOnce(Return(std::nullopt));

    EXPECT_TRUE(closeRequestedOn(backend, kOurWindow));
}

TEST(WindowEventsTest, WindowChanges_ReadsTheLastSizeItWasResizedTo)
{
    NiceMock<MockGfxBackend> backend;

    EXPECT_CALL(backend, pollEvent())
        .WillOnce(Return(resizeOf(kOurWindow)))
        .WillOnce(Return(std::nullopt));

    const auto changes = windowChanges(backend, kOurWindow);

    ASSERT_TRUE(changes.resizedSize.has_value());
    EXPECT_EQ(changes.resizedSize->width, 10U);
    EXPECT_EQ(changes.resizedSize->height, 10U);
    EXPECT_FALSE(changes.closeRequested);
}

TEST(WindowEventsTest, WindowChanges_LeavesTheSizeAloneWithNoResize)
{
    NiceMock<MockGfxBackend> backend;

    EXPECT_CALL(backend, pollEvent())
        .WillOnce(Return(closeOf(kOurWindow)))
        .WillOnce(Return(std::nullopt));

    const auto changes = windowChanges(backend, kOurWindow);

    EXPECT_FALSE(changes.resizedSize.has_value());
    EXPECT_TRUE(changes.closeRequested);
}

TEST(WindowEventsTest, WindowChanges_IgnoresAResizeOfAnotherWindow)
{
    NiceMock<MockGfxBackend> backend;

    EXPECT_CALL(backend, pollEvent())
        .WillOnce(Return(resizeOf(kSomeoneElsesWindow)))
        .WillOnce(Return(std::nullopt));

    EXPECT_FALSE(
        windowChanges(backend, kOurWindow).resizedSize.has_value());
}

TEST(WindowEventsTest, WindowChanges_ReadsACloseAndAResizeTogether)
{
    NiceMock<MockGfxBackend> backend;

    EXPECT_CALL(backend, pollEvent())
        .WillOnce(Return(resizeOf(kOurWindow)))
        .WillOnce(Return(closeOf(kOurWindow)))
        .WillOnce(Return(std::nullopt));

    const auto changes = windowChanges(backend, kOurWindow);

    EXPECT_TRUE(changes.closeRequested);
    EXPECT_TRUE(changes.resizedSize.has_value());
}
