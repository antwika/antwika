#include <gtest/gtest.h>

#include <optional>

#include <antwika/app/PointerReading.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/InputState.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/input/PointerHint.hpp>
#include <antwika/input/Position.hpp>

using antwika::app::asPoint;
using antwika::app::hoverFrom;
using antwika::app::isLeftPress;
using antwika::app::isLeftRelease;
using antwika::app::isPressOf;
using antwika::app::isReleaseOf;
using antwika::app::leftPress;
using antwika::app::pressOf;
using antwika::app::locates;
using antwika::app::pointerFrom;
using antwika::gfx::Point;
using antwika::input::InputEvent;
using antwika::input::InputState;
using antwika::input::PointerHint;
using antwika::input::KeyPressed;
using antwika::input::MouseButton;
using antwika::input::PointerButtonPressed;
using antwika::input::PointerButtonReleased;
using antwika::input::PointerMoved;
using antwika::input::PointerScrolled;
using antwika::input::Position;

TEST(PointerReadingTest, AsPoint_ReadsAPositionAsAPoint)
{
    constexpr Position position{.x = 12, .y = -34};

    EXPECT_EQ(asPoint(position), (Point{.x = 12, .y = -34}));
}

TEST(PointerReadingTest, Locates_IsTrueForAPositionedEvent)
{
    EXPECT_TRUE(locates(PointerMoved{}));
    EXPECT_TRUE(locates(PointerButtonPressed{}));
    EXPECT_TRUE(locates(PointerButtonReleased{}));
}

TEST(PointerReadingTest, Locates_IsFalseForAScrollOrKey)
{
    EXPECT_FALSE(locates(PointerScrolled{}));
    EXPECT_FALSE(locates(KeyPressed{}));
}

TEST(PointerReadingTest, PointerFrom_GivesNoPositionWhenUnlocated)
{
    InputState state;

    const auto pointer = pointerFrom(state, false);

    EXPECT_FALSE(pointer.positionPoint.has_value());
    EXPECT_FALSE(pointer.down);
    EXPECT_FALSE(pointer.pressed);
}

TEST(PointerReadingTest, PointerFrom_ReportsWhereAHeldButtonWentDown)
{
    InputState state;
    state.beginTick();
    state.apply(PointerButtonPressed{
        .button = MouseButton::Left, .position = {.x = 40, .y = 60}});

    const auto pointer = pointerFrom(state, true);

    EXPECT_EQ(pointer.positionPoint, (Point{.x = 40, .y = 60}));
    EXPECT_TRUE(pointer.down);
    EXPECT_TRUE(pointer.pressed);
}

TEST(PointerReadingTest, PointerFrom_ReadsTheButtonItWasAsked)
{
    InputState state;
    state.beginTick();
    state.apply(PointerButtonPressed{
        .button = MouseButton::Right, .position = {.x = 1, .y = 2}});

    const auto left = pointerFrom(state, true, MouseButton::Left);
    const auto right = pointerFrom(state, true, MouseButton::Right);

    EXPECT_FALSE(left.pressed);
    EXPECT_TRUE(right.pressed);
}

TEST(PointerReadingTest, HoverFrom_ReadsAHintAsAHoverPointer)
{
    constexpr PointerHint hint{.position = {.x = 7, .y = 9}};

    EXPECT_EQ(hoverFrom(hint).positionPoint, (Point{.x = 7, .y = 9}));
}

TEST(PointerReadingTest, HoverFrom_ReadsNoHintAsNowhere)
{
    EXPECT_FALSE(hoverFrom(std::nullopt).positionPoint.has_value());
}

TEST(PointerReadingTest, PressOf_AnswersThePressOfTheButtonAsked)
{
    const InputEvent event = PointerButtonPressed{
        .button = MouseButton::Right, .position = {.x = 7, .y = 9}};

    const auto *pressedEvent = pressOf(event, MouseButton::Right);

    ASSERT_NE(pressedEvent, nullptr);
    EXPECT_EQ(pressedEvent->position.x, 7);
}

TEST(PointerReadingTest, PressOf_AnswersNothingForAnotherButton)
{
    const InputEvent event =
        PointerButtonPressed{.button = MouseButton::Right};

    EXPECT_EQ(pressOf(event, MouseButton::Left), nullptr);
}

TEST(PointerReadingTest, PressOf_AnswersNothingForWhatIsNotAPress)
{
    const InputEvent event = PointerMoved{.position = {.x = 1, .y = 2}};

    EXPECT_EQ(pressOf(event, MouseButton::Left), nullptr);
}

TEST(PointerReadingTest, IsPressOf_SaysWhetherTheButtonWentDown)
{
    const InputEvent event =
        PointerButtonPressed{.button = MouseButton::Middle};

    EXPECT_TRUE(isPressOf(event, MouseButton::Middle));
    EXPECT_FALSE(isPressOf(event, MouseButton::Left));
}

TEST(PointerReadingTest, LeftPress_AnswersOnlyTheLeftButtonsPress)
{
    const InputEvent leftEvent =
        PointerButtonPressed{.button = MouseButton::Left};
    const InputEvent rightEvent =
        PointerButtonPressed{.button = MouseButton::Right};

    EXPECT_NE(leftPress(leftEvent), nullptr);
    EXPECT_EQ(leftPress(rightEvent), nullptr);
}

TEST(PointerReadingTest, IsLeftPress_SaysWhetherTheLeftButtonWentDown)
{
    EXPECT_TRUE(isLeftPress(
        InputEvent{PointerButtonPressed{.button = MouseButton::Left}}));
    EXPECT_FALSE(isLeftPress(
        InputEvent{PointerButtonPressed{.button = MouseButton::Right}}));
}

TEST(PointerReadingTest, IsReleaseOf_SaysWhetherTheButtonCameUp)
{
    const InputEvent event =
        PointerButtonReleased{.button = MouseButton::Right};

    EXPECT_TRUE(isReleaseOf(event, MouseButton::Right));
    EXPECT_FALSE(isReleaseOf(event, MouseButton::Left));
}

TEST(PointerReadingTest, IsReleaseOf_SaysNoToWhatIsNotARelease)
{
    EXPECT_FALSE(isReleaseOf(
        InputEvent{PointerMoved{}}, MouseButton::Left));
}

TEST(PointerReadingTest, IsLeftRelease_SaysWhetherTheLeftButtonCameUp)
{
    EXPECT_TRUE(isLeftRelease(
        InputEvent{PointerButtonReleased{.button = MouseButton::Left}}));
    EXPECT_FALSE(isLeftRelease(
        InputEvent{PointerButtonReleased{.button = MouseButton::Right}}));
}
