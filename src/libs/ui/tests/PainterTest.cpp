#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/gfx/RectF.hpp>
#include <antwika/gfx/PointF.hpp>

#include "antwika/ui/DrawCommand.hpp"
#include "antwika/ui/DrawList.hpp"
#include "antwika/ui/Painter.hpp"

using antwika::gfx::Color;
using antwika::gfx::Point;
using antwika::gfx::PointF;
using antwika::gfx::Rect;
using antwika::gfx::RectF;
using antwika::gfx::mocks::MockRenderer;
using antwika::ui::DrawList;
using antwika::ui::DrawText;
using antwika::ui::FillRect;
using antwika::ui::paint;
using ::testing::_;
using ::testing::InSequence;
using ::testing::NiceMock;

namespace
{
    constexpr Color kInk{.red = 220, .green = 224, .blue = 228};
    constexpr Color kPanel{.red = 20, .green = 24, .blue = 30};

    constexpr Rect kBox{
        .origin = {.x = 3, .y = 4},
        .size = {.width = 30, .height = 12}};
}

TEST(PainterTest, Paint_DrawsAFillAsARectangle)
{
    NiceMock<MockRenderer> renderer;

    EXPECT_CALL(renderer, drawRect(RectF{kBox}, kPanel));

    paint(renderer, DrawList{FillRect{.rect = kBox, .color = kPanel}});
}

TEST(PainterTest, Paint_DrawsTextWithItsOwnScaleAndColour)
{
    NiceMock<MockRenderer> renderer;

    EXPECT_CALL(
        renderer, drawText(PointF{3.0F, 4.0F}, "ab", 2, kInk));

    paint(
        renderer,
        DrawList{DrawText{
            .origin = {.x = 3, .y = 4},
            .text = "ab",
            .scale = 2,
            .color = kInk}});
}

TEST(PainterTest, Paint_DrawsCommandsInTheOrderTheyAreGiven)
{
    NiceMock<MockRenderer> renderer;
    const InSequence sequence;

    EXPECT_CALL(renderer, drawRect(RectF{kBox}, kPanel));
    EXPECT_CALL(
        renderer, drawText(PointF{3.0F, 4.0F}, "ab", 1, kInk));
    EXPECT_CALL(renderer, drawRect(RectF{kBox}, kInk));

    paint(
        renderer,
        DrawList{
            FillRect{.rect = kBox, .color = kPanel},
            DrawText{
                .origin = {.x = 3, .y = 4},
                .text = "ab",
                .scale = 1,
                .color = kInk},
            FillRect{.rect = kBox, .color = kInk}});
}

TEST(PainterTest, Paint_NeverClearsAndNeverPresents)
{
    NiceMock<MockRenderer> renderer;

    EXPECT_CALL(renderer, drawRect(RectF{kBox}, kPanel));
    EXPECT_CALL(renderer, clear(_)).Times(0);
    EXPECT_CALL(renderer, present()).Times(0);

    paint(
        renderer,
        DrawList{
            FillRect{.rect = kBox, .color = kPanel},
            DrawText{
                .origin = {.x = 3, .y = 4},
                .text = "ab",
                .scale = 1,
                .color = kInk}});
}

TEST(PainterTest, Paint_DrawsNothingForAnEmptyPicture)
{
    NiceMock<MockRenderer> renderer;

    EXPECT_CALL(renderer, drawRect(_, _)).Times(0);
    EXPECT_CALL(renderer, drawText(_, _, _, _)).Times(0);

    paint(renderer, DrawList{});
}
