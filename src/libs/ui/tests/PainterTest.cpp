#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/gfx/mocks/MockTexture.hpp>
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
using antwika::gfx::TextScale;
using antwika::gfx::mocks::MockRenderer;
using antwika::ui::DrawList;
using antwika::ui::DrawText;
using antwika::ui::DrawTexture;
using antwika::ui::FillRect;
using antwika::ui::paint;
using antwika::ui::PopClip;
using antwika::ui::PushClip;
using ::testing::_;
using ::testing::InSequence;
using ::testing::NiceMock;

namespace
{
    constexpr Color kInkColor{.red = 220, .green = 224, .blue = 228};
    constexpr Color kPanelColor{.red = 20, .green = 24, .blue = 30};

    constexpr Rect kBoxRect{
        .originPoint = {.x = 3, .y = 4},
        .size = {.width = 30, .height = 12}};
}

TEST(PainterTest, Paint_DrawsAFillAsARectangle)
{
    NiceMock<MockRenderer> renderer;

    EXPECT_CALL(renderer, drawRect(RectF{kBoxRect}, kPanelColor));

    paint(renderer, DrawList{FillRect{.rect = kBoxRect, .color = kPanelColor}});
}

TEST(PainterTest, Paint_DrawsTextWithItsOwnScaleAndColor)
{
    NiceMock<MockRenderer> renderer;

    EXPECT_CALL(
        renderer, drawText(
            PointF{3.0F, 4.0F},
            "ab",
            TextScale{.multiplier = 2},
            kInkColor));

    paint(
        renderer,
        DrawList{DrawText{
            .originPoint = {.x = 3, .y = 4},
            .text = "ab",
            .scale = {.multiplier = 2},
            .color = kInkColor}});
}

TEST(PainterTest, Paint_DrawsCommandsInTheOrderTheyAreGiven)
{
    NiceMock<MockRenderer> renderer;
    const InSequence sequence;

    EXPECT_CALL(renderer, drawRect(RectF{kBoxRect}, kPanelColor));
    EXPECT_CALL(
        renderer, drawText(
            PointF{3.0F, 4.0F},
            "ab",
            TextScale{.multiplier = 1},
            kInkColor));
    EXPECT_CALL(renderer, drawRect(RectF{kBoxRect}, kInkColor));

    paint(
        renderer,
        DrawList{
            FillRect{.rect = kBoxRect, .color = kPanelColor},
            DrawText{
                .originPoint = {.x = 3, .y = 4},
                .text = "ab",
                .scale = {.multiplier = 1},
                .color = kInkColor},
            FillRect{.rect = kBoxRect, .color = kInkColor}});
}

TEST(PainterTest, Paint_BeginsAClipOverTheCommandsAfterIt)
{
    NiceMock<MockRenderer> renderer;
    const InSequence sequence;

    EXPECT_CALL(renderer, beginClip(RectF{kBoxRect}));
    EXPECT_CALL(renderer, drawRect(RectF{kBoxRect}, kPanelColor));
    EXPECT_CALL(renderer, endClip());

    paint(
        renderer,
        DrawList{
            PushClip{.rect = kBoxRect},
            FillRect{.rect = kBoxRect, .color = kPanelColor},
            PopClip{}});
}

TEST(PainterTest, Paint_NeverClearsAndNeverPresents)
{
    NiceMock<MockRenderer> renderer;

    EXPECT_CALL(renderer, drawRect(RectF{kBoxRect}, kPanelColor));
    EXPECT_CALL(renderer, clear(_)).Times(0);
    EXPECT_CALL(renderer, present()).Times(0);

    paint(
        renderer,
        DrawList{
            FillRect{.rect = kBoxRect, .color = kPanelColor},
            DrawText{
                .originPoint = {.x = 3, .y = 4},
                .text = "ab",
                .scale = {.multiplier = 1},
                .color = kInkColor}});
}

TEST(PainterTest, Paint_DrawsNothingForAnEmptyPicture)
{
    NiceMock<MockRenderer> renderer;

    EXPECT_CALL(renderer, drawRect(_, _)).Times(0);
    EXPECT_CALL(renderer, drawText(_, _, _, _)).Times(0);

    paint(renderer, DrawList{});
}

TEST(PainterTest, Paint_LaysAPictureOverItsRectangle)
{
    NiceMock<MockRenderer> renderer;
    const antwika::gfx::mocks::MockTexture sheetTexture;

    const DrawList drawList{
        DrawTexture{
            .texture = &sheetTexture,
            .sourceRect =
                Rect{
                    .originPoint = {.x = 16, .y = 0},
                    .size = {.width = 16, .height = 16}},
            .destinationRect = kBoxRect,
            .tintColor = kInkColor}};

    EXPECT_CALL(
        renderer,
        drawTexture(
            ::testing::Ref(sheetTexture),
            RectF(
                antwika::geometry::PointF{16.0F, 0.0F},
                antwika::geometry::SizeF{16.0F, 16.0F}),
            RectF(kBoxRect),
            kInkColor));

    paint(renderer, drawList);
}

TEST(PainterTest, Paint_LeavesAPictureWithNoSheetAlone)
{
    NiceMock<MockRenderer> renderer;

    EXPECT_CALL(renderer, drawTexture(_, _, _, _)).Times(0);

    paint(renderer, DrawList{DrawTexture{.destinationRect = kBoxRect}});
}
