#include <gtest/gtest.h>

#include "antwika/input/PointerHint.hpp"

using antwika::input::PointerHint;

namespace
{
    constexpr PointerHint kReference{.position = {.x = 10, .y = 20}};
}

TEST(PointerHintTest, Ctor_IsTheOrigin)
{
    constexpr PointerHint origin;

    EXPECT_EQ(origin.position.x, 0);
    EXPECT_EQ(origin.position.y, 0);
}

TEST(PointerHintTest, OperatorEquals_IsTrueForTheSamePosition)
{
    constexpr PointerHint same{.position = {.x = 10, .y = 20}};

    EXPECT_EQ(kReference, same);
}

TEST(PointerHintTest, OperatorEquals_IsFalseForADifferentPosition)
{
    constexpr PointerHint other{.position = {.x = 10, .y = 99}};

    EXPECT_NE(kReference, other);
}
