#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/mocks/MockWindow.hpp>
#include <antwika/input/Position.hpp>

#include "antwika/app/WindowPointerMapping.hpp"

using antwika::app::WindowPointerMapping;
using antwika::gfx::Size;
using antwika::gfx::mocks::MockWindow;
using antwika::input::Position;
using ::testing::NiceMock;
using ::testing::Return;

namespace
{
    constexpr Size kCanvasSize{.width = 1024, .height = 640};
}

TEST(WindowPointerMappingTest, ToSurface_ChangesNothingUnderTheNullCase)
{
    NiceMock<MockWindow> window;
    ON_CALL(window, getSize()).WillByDefault(Return(kCanvasSize));

    const WindowPointerMapping mapping(window, kCanvasSize);

    EXPECT_EQ(
        mapping.toCanvas(Position{.x = 300, .y = 200}),
        (Position{.x = 300, .y = 200}));
}

TEST(WindowPointerMappingTest, ToSurface_UndoesTheScaleTheViewportApplies)
{
    NiceMock<MockWindow> window;
    ON_CALL(window, getSize())
        .WillByDefault(Return(Size{.width = 2048, .height = 1280}));

    const WindowPointerMapping mapping(window, kCanvasSize);

    EXPECT_EQ(
        mapping.toCanvas(Position{.x = 600, .y = 400}),
        (Position{.x = 300, .y = 200}));
}

TEST(WindowPointerMappingTest, ToSurface_SubtractsTheOffsetOfTheBars)
{
    NiceMock<MockWindow> window;
    ON_CALL(window, getSize())
        .WillByDefault(Return(Size{.width = 1920, .height = 640}));

    const WindowPointerMapping mapping(window, kCanvasSize);

    EXPECT_EQ(
        mapping.toCanvas(Position{.x = 448, .y = 0}),
        (Position{.x = 0, .y = 0}));
    EXPECT_EQ(
        mapping.toCanvas(Position{.x = 100, .y = 10}),
        (Position{.x = -348, .y = 10}));
}

TEST(WindowPointerMappingTest, ToSurface_FollowsAWindowThatChangedSize)
{
    NiceMock<MockWindow> window;
    ON_CALL(window, getSize()).WillByDefault(Return(kCanvasSize));

    const WindowPointerMapping mapping(window, kCanvasSize);

    EXPECT_EQ(
        mapping.toCanvas(Position{.x = 512, .y = 320}),
        (Position{.x = 512, .y = 320}));

    ON_CALL(window, getSize())
        .WillByDefault(Return(Size{.width = 2048, .height = 1280}));

    EXPECT_EQ(
        mapping.toCanvas(Position{.x = 512, .y = 320}),
        (Position{.x = 256, .y = 160}));
}
