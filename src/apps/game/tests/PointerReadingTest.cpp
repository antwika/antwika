#include <gtest/gtest.h>

#include <antwika/gfx/Point.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/Position.hpp>

#include "antwika/game/PointerReading.hpp"

using antwika::game::asPoint;
using antwika::game::locates;
using antwika::gfx::Point;
using antwika::input::KeyPressed;
using antwika::input::PointerButtonPressed;
using antwika::input::PointerButtonReleased;
using antwika::input::PointerMoved;
using antwika::input::PointerScrolled;
using antwika::input::Position;

TEST(PointerReadingTest, AsPoint_KeepsBothCoordinates)
{
    constexpr Position position{.x = 12, .y = -34};

    EXPECT_EQ(asPoint(position), (Point{.x = 12, .y = -34}));
}

TEST(PointerReadingTest, Locates_IsTrueForEveryEventCarryingAPosition)
{
    EXPECT_TRUE(locates(PointerMoved{}));
    EXPECT_TRUE(locates(PointerButtonPressed{}));
    EXPECT_TRUE(locates(PointerButtonReleased{}));
}

// A scroll is the trap: it moves the wheel, not the pointer.
TEST(PointerReadingTest, Locates_IsFalseForEventsCarryingNoPosition)
{
    EXPECT_FALSE(locates(PointerScrolled{}));
    EXPECT_FALSE(locates(KeyPressed{}));
}
