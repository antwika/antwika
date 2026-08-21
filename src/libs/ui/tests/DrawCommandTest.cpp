#include <gtest/gtest.h>

#include <antwika/gfx/Color.hpp>

#include "antwika/ui/DrawCommand.hpp"

using antwika::gfx::Color;
using antwika::ui::DrawText;
using antwika::ui::FillRect;
using antwika::ui::PopClip;
using antwika::ui::PushClip;

namespace
{
    constexpr Color kInkColor{.red = 200, .green = 210, .blue = 220};
    constexpr Color kPanelColor{.red = 20, .green = 24, .blue = 30};

    constexpr FillRect kFill{
        .rect =
            {.originPoint = {.x = 1, .y = 2},
             .size = {.width = 3, .height = 4}},
        .color = kPanelColor};

    const DrawText kText{
        .originPoint = {.x = 5, .y = 6},
        .text = "ab",
        .scale = 2,
        .color = kInkColor};
}

TEST(DrawCommandTest, FillRectEquality_IsTrueForTheSameRectAndColor)
{
    constexpr FillRect sameRect{.rect = kFill.rect, .color = kPanelColor};

    EXPECT_EQ(kFill, sameRect);
}

TEST(DrawCommandTest, FillRectEquality_IsFalseWhenTheRectDiffers)
{
    constexpr FillRect otherRect{
        .rect =
            {.originPoint = {.x = 9, .y = 2},
             .size = {.width = 3, .height = 4}},
        .color = kPanelColor};

    EXPECT_NE(kFill, otherRect);
}

TEST(DrawCommandTest, FillRectEquality_IsFalseWhenTheColorDiffers)
{
    constexpr FillRect otherRect{.rect = kFill.rect, .color = kInkColor};

    EXPECT_NE(kFill, otherRect);
}

TEST(DrawCommandTest, DrawTextEquality_IsTrueForTheSameFields)
{
    const DrawText sameText{
        .originPoint = {.x = 5, .y = 6},
        .text = "ab",
        .scale = 2,
        .color = kInkColor};

    EXPECT_EQ(kText, sameText);
}

TEST(DrawCommandTest, DrawTextEquality_IsFalseWhenTheOriginDiffers)
{
    const DrawText otherText{
        .originPoint = {.x = 9, .y = 6},
        .text = "ab",
        .scale = 2,
        .color = kInkColor};

    EXPECT_NE(kText, otherText);
}

TEST(DrawCommandTest, DrawTextEquality_IsFalseWhenTheTextDiffers)
{
    const DrawText otherText{
        .originPoint = {.x = 5, .y = 6},
        .text = "cd",
        .scale = 2,
        .color = kInkColor};

    EXPECT_NE(kText, otherText);
}

TEST(DrawCommandTest, DrawTextEquality_IsFalseWhenTheScaleDiffers)
{
    const DrawText otherText{
        .originPoint = {.x = 5, .y = 6},
        .text = "ab",
        .scale = 3,
        .color = kInkColor};

    EXPECT_NE(kText, otherText);
}

TEST(DrawCommandTest, DrawTextEquality_IsFalseWhenTheColorDiffers)
{
    const DrawText otherText{
        .originPoint = {.x = 5, .y = 6},
        .text = "ab",
        .scale = 2,
        .color = kPanelColor};

    EXPECT_NE(kText, otherText);
}

TEST(DrawCommandTest, PushClipEquality_IsTrueForTheSameRect)
{
    constexpr PushClip clip{.rect = kFill.rect};
    constexpr PushClip sameClip{.rect = kFill.rect};

    EXPECT_EQ(clip, sameClip);
}

TEST(DrawCommandTest, PushClipEquality_IsFalseWhenTheRectDiffers)
{
    constexpr PushClip clip{.rect = kFill.rect};
    constexpr PushClip otherClip{
        .rect =
            {.originPoint = {.x = 9, .y = 2},
             .size = {.width = 3, .height = 4}}};

    EXPECT_NE(clip, otherClip);
}

TEST(DrawCommandTest, PopClipEquality_IsAlwaysTrue)
{
    EXPECT_EQ(PopClip{}, PopClip{});
}
