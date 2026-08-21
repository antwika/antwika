#include <gtest/gtest.h>

#include "antwika/input/PointerHint.hpp"

using antwika::input::PointerHint;

namespace
{
    constexpr PointerHint kReferenceHint{.position = {.x = 10, .y = 20}};
}

TEST(PointerHintTest, Ctor_IsTheOrigin)
{
    constexpr PointerHint originPointHint;

    EXPECT_EQ(originPointHint.position.x, 0);
    EXPECT_EQ(originPointHint.position.y, 0);
}

TEST(PointerHintTest, OperatorEquals_IsTrueForTheSamePosition)
{
    constexpr PointerHint sameHint{.position = {.x = 10, .y = 20}};

    EXPECT_EQ(kReferenceHint, sameHint);
}

TEST(PointerHintTest, OperatorEquals_IsFalseForADifferentPosition)
{
    constexpr PointerHint otherHint{.position = {.x = 10, .y = 99}};

    EXPECT_NE(kReferenceHint, otherHint);
}
