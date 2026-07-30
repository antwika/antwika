#include <gtest/gtest.h>

#include "antwika/input/Mouse.hpp"
#include "antwika/input/MouseButton.hpp"
#include "antwika/input/Offset.hpp"
#include "antwika/input/Position.hpp"

using antwika::input::Mouse;
using antwika::input::MouseButton;
using antwika::input::Offset;
using antwika::input::PointerButtonPressed;
using antwika::input::PointerButtonReleased;
using antwika::input::PointerMoved;
using antwika::input::PointerScrolled;
using antwika::input::Position;

namespace
{
    // Past the last enumerator, so every guard has something to reject.
    constexpr auto kUnnamedButton = static_cast<MouseButton>(200);
} // namespace

TEST(MouseTest, Position_ReportsTheOriginBeforeAnyEvent)
{
    const Mouse mouse;

    EXPECT_EQ(mouse.position(), Position{});
    EXPECT_EQ(mouse.delta(), Offset{});
    EXPECT_EQ(mouse.scroll(), Offset{});
    EXPECT_FALSE(mouse.isDown(MouseButton::Left));
}

TEST(MouseTest, Apply_ReportsNoDeltaForTheFirstPositionItLearns)
{
    Mouse mouse;

    mouse.apply(PointerMoved{.position = {.x = 400, .y = 300}});

    EXPECT_EQ(mouse.position(), (Position{.x = 400, .y = 300}));
    EXPECT_EQ(mouse.delta(), Offset{});
}

TEST(MouseTest, Apply_ReportsTheDeltaBetweenTwoPositions)
{
    Mouse mouse;
    mouse.apply(PointerMoved{.position = {.x = 400, .y = 300}});

    mouse.apply(PointerMoved{.position = {.x = 410, .y = 290}});

    EXPECT_EQ(mouse.position(), (Position{.x = 410, .y = 290}));
    EXPECT_EQ(mouse.delta(), (Offset{.x = 10, .y = -10}));
}

TEST(MouseTest, Apply_SumsEveryMovementWithinOneTick)
{
    Mouse mouse;
    mouse.apply(PointerMoved{.position = {.x = 0, .y = 0}});

    mouse.apply(PointerMoved{.position = {.x = 3, .y = 1}});
    mouse.apply(PointerMoved{.position = {.x = 8, .y = 4}});

    EXPECT_EQ(mouse.delta(), (Offset{.x = 8, .y = 4}));
}

TEST(MouseTest, Apply_MarksAPressedButtonHeldAndPressed)
{
    Mouse mouse;

    mouse.apply(
        PointerButtonPressed{
            .button = MouseButton::Right, .position = {.x = 5, .y = 6}});

    EXPECT_TRUE(mouse.isDown(MouseButton::Right));
    EXPECT_TRUE(mouse.wasPressed(MouseButton::Right));
    EXPECT_FALSE(mouse.wasReleased(MouseButton::Right));
    EXPECT_FALSE(mouse.isDown(MouseButton::Left));
}

TEST(MouseTest, Apply_MarksAReleasedButtonNoLongerHeld)
{
    Mouse mouse;
    mouse.apply(PointerButtonPressed{.button = MouseButton::Middle});

    mouse.apply(PointerButtonReleased{.button = MouseButton::Middle});

    EXPECT_FALSE(mouse.isDown(MouseButton::Middle));
    EXPECT_TRUE(mouse.wasReleased(MouseButton::Middle));
}

TEST(MouseTest, Apply_TakesThePositionAPressReports)
{
    Mouse mouse;

    mouse.apply(
        PointerButtonPressed{
            .button = MouseButton::Left, .position = {.x = 12, .y = 34}});

    EXPECT_EQ(mouse.position(), (Position{.x = 12, .y = 34}));
    EXPECT_EQ(mouse.delta(), Offset{});
}

TEST(MouseTest, Apply_TakesThePositionAReleaseReports)
{
    Mouse mouse;
    mouse.apply(PointerMoved{.position = {.x = 10, .y = 10}});

    mouse.apply(
        PointerButtonReleased{
            .button = MouseButton::Left, .position = {.x = 14, .y = 10}});

    EXPECT_EQ(mouse.position(), (Position{.x = 14, .y = 10}));
    EXPECT_EQ(mouse.delta(), (Offset{.x = 4, .y = 0}));
}

TEST(MouseTest, Apply_SumsScrollNotchesOnBothAxes)
{
    Mouse mouse;

    mouse.apply(PointerScrolled{.horizontal = 1, .vertical = 2});
    mouse.apply(PointerScrolled{.horizontal = -3, .vertical = 1});

    EXPECT_EQ(mouse.scroll(), (Offset{.x = -2, .y = 3}));
}

TEST(MouseTest, BeginTick_ClearsTheEdgesTheDeltaAndTheScroll)
{
    Mouse mouse;
    mouse.apply(PointerMoved{.position = {.x = 0, .y = 0}});
    mouse.apply(PointerMoved{.position = {.x = 9, .y = 9}});
    mouse.apply(PointerButtonPressed{.button = MouseButton::Left});
    mouse.apply(PointerScrolled{.vertical = 4});

    mouse.beginTick();

    EXPECT_EQ(mouse.delta(), Offset{});
    EXPECT_EQ(mouse.scroll(), Offset{});
    EXPECT_FALSE(mouse.wasPressed(MouseButton::Left));
}

TEST(MouseTest, BeginTick_KeepsTheButtonHeldAndThePosition)
{
    Mouse mouse;
    mouse.apply(
        PointerButtonPressed{
            .button = MouseButton::Left, .position = {.x = 7, .y = 8}});

    mouse.beginTick();

    EXPECT_TRUE(mouse.isDown(MouseButton::Left));
    EXPECT_EQ(mouse.position(), (Position{.x = 7, .y = 8}));
}

TEST(MouseTest, BeginTick_KeepsTheDeltaMeasuredFromTheLastPosition)
{
    Mouse mouse;
    mouse.apply(PointerMoved{.position = {.x = 100, .y = 100}});
    mouse.beginTick();

    mouse.apply(PointerMoved{.position = {.x = 105, .y = 100}});

    EXPECT_EQ(mouse.delta(), (Offset{.x = 5, .y = 0}));
}

TEST(MouseTest, BeginTick_ClearsAReleaseEdge)
{
    Mouse mouse;
    mouse.apply(PointerButtonReleased{.button = MouseButton::Left});

    mouse.beginTick();

    EXPECT_FALSE(mouse.wasReleased(MouseButton::Left));
}

TEST(MouseTest, Apply_IgnoresAButtonOutsideTheEnumeration)
{
    Mouse mouse;

    mouse.apply(PointerButtonPressed{.button = kUnnamedButton});
    mouse.apply(PointerButtonReleased{.button = kUnnamedButton});

    EXPECT_FALSE(mouse.isDown(kUnnamedButton));
    EXPECT_FALSE(mouse.wasPressed(kUnnamedButton));
    EXPECT_FALSE(mouse.wasReleased(kUnnamedButton));
}

TEST(MouseTest, Apply_StillTakesThePositionOfAnUnnamedButtonPress)
{
    Mouse mouse;

    mouse.apply(
        PointerButtonPressed{
            .button = kUnnamedButton, .position = {.x = 3, .y = 4}});

    EXPECT_EQ(mouse.position(), (Position{.x = 3, .y = 4}));
}

TEST(MouseTest, Apply_StillTakesThePositionOfAnUnnamedButtonRelease)
{
    Mouse mouse;

    mouse.apply(
        PointerButtonReleased{
            .button = kUnnamedButton, .position = {.x = 5, .y = 6}});

    EXPECT_EQ(mouse.position(), (Position{.x = 5, .y = 6}));
}
