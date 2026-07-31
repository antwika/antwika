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
using antwika::app::locates;
using antwika::app::pointerFrom;
using antwika::gfx::Point;
using antwika::input::InputState;
using antwika::input::PointerHint;
using antwika::input::KeyPressed;
using antwika::input::MouseButton;
using antwika::input::PointerButtonPressed;
using antwika::input::PointerButtonReleased;
using antwika::input::PointerMoved;
using antwika::input::PointerScrolled;
using antwika::input::Position;

TEST(PointerReadingTest, ReadsAPositionAsAPoint)
{
    constexpr Position position{.x = 12, .y = -34};

    EXPECT_EQ(asPoint(position), (Point{.x = 12, .y = -34}));
}

TEST(PointerReadingTest, PointerEventsCarryingAPositionLocateIt)
{
    EXPECT_TRUE(locates(PointerMoved{}));
    EXPECT_TRUE(locates(PointerButtonPressed{}));
    EXPECT_TRUE(locates(PointerButtonReleased{}));
}

TEST(PointerReadingTest, AScrollOrAKeyLocatesNothing)
{
    EXPECT_FALSE(locates(PointerScrolled{}));
    EXPECT_FALSE(locates(KeyPressed{}));
}

TEST(PointerReadingTest, AnUnlocatedPointerHasNoPosition)
{
    InputState state;

    const auto pointer = pointerFrom(state, false);

    EXPECT_FALSE(pointer.position.has_value());
    EXPECT_FALSE(pointer.down);
    EXPECT_FALSE(pointer.pressed);
}

TEST(PointerReadingTest, ReportsWhereAHeldButtonWentDown)
{
    InputState state;
    state.beginTick();
    state.apply(PointerButtonPressed{
        .button = MouseButton::Left, .position = {.x = 40, .y = 60}});

    const auto pointer = pointerFrom(state, true);

    EXPECT_EQ(pointer.position, (Point{.x = 40, .y = 60}));
    EXPECT_TRUE(pointer.down);
    EXPECT_TRUE(pointer.pressed);
}

TEST(PointerReadingTest, ReadsTheButtonItWasAskedAbout)
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

// A hint is where a free-moving pointer is, unreproduced by a replay.
// So it converts to the one type a UI cannot act on.
TEST(PointerReadingTest, ReadsAPointerHintAsAHoverPointer)
{
    constexpr PointerHint hint{.position = {.x = 7, .y = 9}};

    EXPECT_EQ(hoverFrom(hint).position, (Point{.x = 7, .y = 9}));
}

TEST(PointerReadingTest, ReadsNoHintAsANoWhereHoverPointer)
{
    // Nothing has said where the pointer is.
    // A nowhere hover leaves a picture as it was.
    // An origin would light whatever sits in the corner.
    EXPECT_FALSE(hoverFrom(std::nullopt).position.has_value());
}
