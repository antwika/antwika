#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/gfx/RectF.hpp>

#include "antwika/life/BoardScene.hpp"
#include "antwika/life/Board.hpp"

using antwika::gfx::Color;
using antwika::gfx::Rect;
using antwika::gfx::RectF;
using antwika::gfx::Size;
using antwika::gfx::mocks::MockRenderer;
using antwika::life::Board;
using antwika::life::BoardScene;
using ::testing::_;
using ::testing::InSequence;

namespace
{
    constexpr Color kBackground{.red = 16, .green = 16, .blue = 24};
    constexpr Color kDeadCells{.red = 32, .green = 36, .blue = 48};
    constexpr Color kAliveCell{.red = 96, .green = 224, .blue = 128};
}

TEST(BoardSceneTest, Draw_ClearsThenFillsTheBoardAreaThenEachAliveCell)
{
    MockRenderer renderer;
    const BoardScene scene;
    const InSequence sequence;

    const Board board{
        .width = 3,
        .height = 2,
        .alive = {false, true, false, true, false, false}};

    EXPECT_CALL(renderer, clear(kBackground));

    EXPECT_CALL(
        renderer,
        drawRect(
            RectF{Rect{
                .origin = {.x = 0, .y = 0},
                .size = {.width = 30, .height = 20}}},
            kDeadCells));

    EXPECT_CALL(
        renderer,
        drawRect(
            RectF{Rect{
                .origin = {.x = 10, .y = 0},
                .size = {.width = 10, .height = 10}}},
            kAliveCell));

    EXPECT_CALL(
        renderer,
        drawRect(
            RectF{Rect{
                .origin = {.x = 0, .y = 10},
                .size = {.width = 10, .height = 10}}},
            kAliveCell));

    scene.draw(renderer, Size{.width = 30, .height = 20}, board);
}

TEST(BoardSceneTest, Draw_SizesCellsToTheShorterCanvasAxisOnAWideCanvas)
{
    MockRenderer renderer;
    const BoardScene scene;
    const Board board{
        .width = 2, .height = 2, .alive = {false, false, false, false}};

    EXPECT_CALL(renderer, clear(_));
    EXPECT_CALL(
        renderer,
        drawRect(
            RectF{Rect{
                .origin = {.x = 50, .y = 0},
                .size = {.width = 100, .height = 100}}},
            kDeadCells));

    scene.draw(renderer, Size{.width = 200, .height = 100}, board);
}

TEST(BoardSceneTest, Draw_SizesCellsToTheShorterCanvasAxisOnATallCanvas)
{
    MockRenderer renderer;
    const BoardScene scene;
    const Board board{
        .width = 2, .height = 2, .alive = {false, false, false, false}};

    EXPECT_CALL(renderer, clear(_));
    EXPECT_CALL(
        renderer,
        drawRect(
            RectF{Rect{
                .origin = {.x = 0, .y = 50},
                .size = {.width = 100, .height = 100}}},
            kDeadCells));

    scene.draw(renderer, Size{.width = 100, .height = 200}, board);
}

TEST(BoardSceneTest, Draw_CentresTheBoardWithinTheCanvas)
{
    MockRenderer renderer;
    const BoardScene scene;
    const Board board{
        .width = 3, .height = 3, .alive = std::vector<bool>(9, true)};

    EXPECT_CALL(renderer, clear(_));
    EXPECT_CALL(
        renderer,
        drawRect(
            RectF{Rect{
                .origin = {.x = 1, .y = 6},
                .size = {.width = 48, .height = 48}}},
            kDeadCells));
    EXPECT_CALL(renderer, drawRect(_, kAliveCell)).Times(9);

    scene.draw(renderer, Size{.width = 50, .height = 60}, board);
}

TEST(BoardSceneTest, Draw_OnlyClearsWhenTheBoardHasNoColumns)
{
    MockRenderer renderer;
    const BoardScene scene;

    EXPECT_CALL(renderer, clear(kBackground));
    EXPECT_CALL(renderer, drawRect(_, _)).Times(0);

    scene.draw(
        renderer,
        Size{.width = 100, .height = 100},
        Board{.width = 0, .height = 2, .alive = {}});
}

TEST(BoardSceneTest, Draw_OnlyClearsWhenTheBoardHasNoRows)
{
    MockRenderer renderer;
    const BoardScene scene;

    EXPECT_CALL(renderer, clear(kBackground));
    EXPECT_CALL(renderer, drawRect(_, _)).Times(0);

    scene.draw(
        renderer,
        Size{.width = 100, .height = 100},
        Board{.width = 2, .height = 0, .alive = {}});
}

TEST(BoardSceneTest, Draw_OnlyClearsWhenTheCanvasIsSmallerThanOneCell)
{
    MockRenderer renderer;
    const BoardScene scene;
    const Board board{
        .width = 4, .height = 4, .alive = std::vector<bool>(16, true)};

    EXPECT_CALL(renderer, clear(kBackground));
    EXPECT_CALL(renderer, drawRect(_, _)).Times(0);

    scene.draw(renderer, Size{.width = 3, .height = 3}, board);
}
