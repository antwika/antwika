#include <gtest/gtest.h>

#include "antwika/gfx/Rect.hpp"

using antwika::gfx::Rect;

namespace
{
    constexpr Rect kReferenceRect{
        .originPoint = {.x = 10, .y = 20},
        .size = {.width = 30, .height = 40}};
}

TEST(RectTest, OperatorEquals_IsTrueForIdenticalOriginAndSize)
{
    constexpr Rect sameRect{
        .originPoint = {.x = 10, .y = 20},
        .size = {.width = 30, .height = 40}};

    EXPECT_EQ(kReferenceRect, sameRect);
}

TEST(RectTest, OperatorEquals_IsFalseWhenTheOriginDiffers)
{
    constexpr Rect otherRect{
        .originPoint = {.x = 99, .y = 20},
        .size = {.width = 30, .height = 40}};

    EXPECT_NE(kReferenceRect, otherRect);
}

TEST(RectTest, OperatorEquals_IsFalseWhenTheSizeDiffers)
{
    constexpr Rect otherRect{
        .originPoint = {.x = 10, .y = 20},
        .size = {.width = 99, .height = 40}};

    EXPECT_NE(kReferenceRect, otherRect);
}
