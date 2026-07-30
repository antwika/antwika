#include <gtest/gtest.h>

#include <antwika/gfx/Color.hpp>

#include "antwika/ui/DrawCommand.hpp"

using antwika::gfx::Color;
using antwika::ui::DrawText;
using antwika::ui::FillRect;

namespace
{
    constexpr Color kInk{.red = 200, .green = 210, .blue = 220};
    constexpr Color kPanel{.red = 20, .green = 24, .blue = 30};

    constexpr FillRect kFill{
        .rect =
            {.origin = {.x = 1, .y = 2},
             .size = {.width = 3, .height = 4}},
        .color = kPanel};

    const DrawText kText{
        .origin = {.x = 5, .y = 6},
        .text = "ab",
        .scale = 2,
        .color = kInk};
} // namespace

TEST(DrawCommandTest, FillRectEquality_IsTrueForTheSameRectAndColour)
{
    constexpr FillRect same{.rect = kFill.rect, .color = kPanel};

    EXPECT_EQ(kFill, same);
}

TEST(DrawCommandTest, FillRectEquality_IsFalseWhenTheRectDiffers)
{
    constexpr FillRect other{
        .rect =
            {.origin = {.x = 9, .y = 2},
             .size = {.width = 3, .height = 4}},
        .color = kPanel};

    EXPECT_NE(kFill, other);
}

TEST(DrawCommandTest, FillRectEquality_IsFalseWhenTheColourDiffers)
{
    constexpr FillRect other{.rect = kFill.rect, .color = kInk};

    EXPECT_NE(kFill, other);
}

TEST(DrawCommandTest, DrawTextEquality_IsTrueForTheSameFields)
{
    const DrawText same{
        .origin = {.x = 5, .y = 6},
        .text = "ab",
        .scale = 2,
        .color = kInk};

    EXPECT_EQ(kText, same);
}

TEST(DrawCommandTest, DrawTextEquality_IsFalseWhenTheOriginDiffers)
{
    const DrawText other{
        .origin = {.x = 9, .y = 6},
        .text = "ab",
        .scale = 2,
        .color = kInk};

    EXPECT_NE(kText, other);
}

TEST(DrawCommandTest, DrawTextEquality_IsFalseWhenTheTextDiffers)
{
    const DrawText other{
        .origin = {.x = 5, .y = 6},
        .text = "cd",
        .scale = 2,
        .color = kInk};

    EXPECT_NE(kText, other);
}

TEST(DrawCommandTest, DrawTextEquality_IsFalseWhenTheScaleDiffers)
{
    const DrawText other{
        .origin = {.x = 5, .y = 6},
        .text = "ab",
        .scale = 3,
        .color = kInk};

    EXPECT_NE(kText, other);
}

TEST(DrawCommandTest, DrawTextEquality_IsFalseWhenTheColourDiffers)
{
    const DrawText other{
        .origin = {.x = 5, .y = 6},
        .text = "ab",
        .scale = 2,
        .color = kPanel};

    EXPECT_NE(kText, other);
}
