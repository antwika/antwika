#include <gtest/gtest.h>

#include "antwika/gfx/Rect.hpp"

using antwika::gfx::Rect;

namespace
{
    constexpr Rect kReference{
        .origin = {.x = 10, .y = 20},
        .size = {.width = 30, .height = 40}};
} // namespace

TEST(RectTest, Equality_IsTrueForIdenticalOriginAndSize)
{
    constexpr Rect same{
        .origin = {.x = 10, .y = 20},
        .size = {.width = 30, .height = 40}};

    EXPECT_EQ(kReference, same);
}

TEST(RectTest, Equality_IsFalseWhenTheOriginDiffers)
{
    constexpr Rect other{
        .origin = {.x = 99, .y = 20},
        .size = {.width = 30, .height = 40}};

    EXPECT_NE(kReference, other);
}

TEST(RectTest, Equality_IsFalseWhenTheSizeDiffers)
{
    constexpr Rect other{
        .origin = {.x = 10, .y = 20},
        .size = {.width = 99, .height = 40}};

    EXPECT_NE(kReference, other);
}
