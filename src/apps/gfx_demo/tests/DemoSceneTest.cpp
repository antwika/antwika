#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>

#include "antwika/gfx_demo/DemoScene.hpp"

using antwika::gfx::Color;
using antwika::gfx::Rect;
using antwika::gfx::Size;
using antwika::gfx::mocks::MockRenderer;
using antwika::gfx_demo::DemoScene;
using ::testing::InSequence;

TEST(DemoSceneTest, Draw_ClearsThenDrawsOneBarPerColourInOrder)
{
    MockRenderer renderer;
    const DemoScene scene;
    const InSequence sequence;

    EXPECT_CALL(
        renderer,
        clear(Color{.red = 16, .green = 16, .blue = 24}));

    EXPECT_CALL(
        renderer,
        drawRect(
            Rect{
                .origin = {.x = 100, .y = 100},
                .size = {.width = 100, .height = 200}},
            Color{.red = 224, .green = 64, .blue = 64}));

    EXPECT_CALL(
        renderer,
        drawRect(
            Rect{
                .origin = {.x = 300, .y = 100},
                .size = {.width = 100, .height = 200}},
            Color{.red = 64, .green = 224, .blue = 96}));

    EXPECT_CALL(
        renderer,
        drawRect(
            Rect{
                .origin = {.x = 500, .y = 100},
                .size = {.width = 100, .height = 200}},
            Color{.red = 80, .green = 128, .blue = 240}));

    scene.draw(renderer, Size{.width = 700, .height = 400});
}

TEST(DemoSceneTest, Draw_ScalesTheBarsToTheCanvas)
{
    MockRenderer renderer;
    const DemoScene scene;

    EXPECT_CALL(renderer, clear(::testing::_));

    EXPECT_CALL(
        renderer,
        drawRect(
            Rect{
                .origin = {.x = 200, .y = 200},
                .size = {.width = 200, .height = 400}},
            ::testing::_));

    EXPECT_CALL(
        renderer,
        drawRect(
            Rect{
                .origin = {.x = 600, .y = 200},
                .size = {.width = 200, .height = 400}},
            ::testing::_));

    EXPECT_CALL(
        renderer,
        drawRect(
            Rect{
                .origin = {.x = 1000, .y = 200},
                .size = {.width = 200, .height = 400}},
            ::testing::_));

    scene.draw(renderer, Size{.width = 1400, .height = 800});
}
