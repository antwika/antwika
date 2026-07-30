#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/gfx/mocks/MockTexture.hpp>

#include "antwika/gfx_demo/DemoScene.hpp"

using antwika::gfx::Color;
using antwika::gfx::Rect;
using antwika::gfx::Size;
using antwika::gfx::mocks::MockRenderer;
using antwika::gfx::mocks::MockTexture;
using antwika::gfx_demo::DemoScene;
using ::testing::_;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Ref;
using ::testing::Return;

namespace
{
    // A 64x64 logo, so halving its width is exactly 32.
    constexpr Size kLogoSize{.width = 64, .height = 64};

    constexpr Color kUntinted{
        .red = 255, .green = 255, .blue = 255, .alpha = 255};

    constexpr Color kWarmTint{
        .red = 255, .green = 96, .blue = 96, .alpha = 255};
} // namespace

class DemoSceneTest : public ::testing::Test
{
protected:
    DemoSceneTest()
    {
        ON_CALL(logo, size()).WillByDefault(Return(kLogoSize));
    }

    MockRenderer renderer;
    NiceMock<MockTexture> logo;
    DemoScene scene;
};

TEST_F(DemoSceneTest, Draw_ClearsThenDrawsOneBarPerColourInOrder)
{
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

    EXPECT_CALL(renderer, drawTexture(_, _, _, _)).Times(2);

    scene.draw(renderer, Size{.width = 700, .height = 400}, logo);
}

TEST_F(DemoSceneTest, Draw_BlitsTheWholeLogoUntintedAboveTheBars)
{
    EXPECT_CALL(renderer, clear(_));
    EXPECT_CALL(renderer, drawRect(_, _)).Times(3);
    EXPECT_CALL(renderer, drawTexture(_, _, _, kWarmTint));

    // A 700x400 canvas gives a 50px badge at x=(700-50)/2, y=400/16.
    EXPECT_CALL(
        renderer,
        drawTexture(
            Ref(logo),
            Rect{
                .origin = {.x = 0, .y = 0},
                .size = {.width = 64, .height = 64}},
            Rect{
                .origin = {.x = 325, .y = 25},
                .size = {.width = 50, .height = 50}},
            kUntinted));

    scene.draw(renderer, Size{.width = 700, .height = 400}, logo);
}

TEST_F(DemoSceneTest, Draw_BlitsTheLogosLeftHalfTintedBelowTheBars)
{
    EXPECT_CALL(renderer, clear(_));
    EXPECT_CALL(renderer, drawRect(_, _)).Times(3);
    EXPECT_CALL(renderer, drawTexture(_, _, _, kUntinted));

    // Half the source into the same badge as the untinted blit.
    // That difference is what makes a source rectangle visible.
    EXPECT_CALL(
        renderer,
        drawTexture(
            Ref(logo),
            Rect{
                .origin = {.x = 0, .y = 0},
                .size = {.width = 32, .height = 64}},
            Rect{
                .origin = {.x = 325, .y = 325},
                .size = {.width = 50, .height = 50}},
            kWarmTint));

    scene.draw(renderer, Size{.width = 700, .height = 400}, logo);
}

TEST_F(DemoSceneTest, Draw_AsksTheTextureForItsSizeRatherThanAssuming)
{
    // A different logo must change the source rectangles.
    // The scene has no idea what it was handed until it asks.
    EXPECT_CALL(logo, size())
        .WillRepeatedly(Return(Size{.width = 20, .height = 10}));

    EXPECT_CALL(renderer, clear(_));
    EXPECT_CALL(renderer, drawRect(_, _)).Times(3);

    EXPECT_CALL(
        renderer,
        drawTexture(
            _,
            Rect{
                .origin = {.x = 0, .y = 0},
                .size = {.width = 20, .height = 10}},
            _, kUntinted));

    EXPECT_CALL(
        renderer,
        drawTexture(
            _,
            Rect{
                .origin = {.x = 0, .y = 0},
                .size = {.width = 10, .height = 10}},
            _, kWarmTint));

    scene.draw(renderer, Size{.width = 700, .height = 400}, logo);
}

TEST_F(DemoSceneTest, Draw_ScalesTheBarsToTheCanvas)
{
    EXPECT_CALL(renderer, clear(::testing::_));
    EXPECT_CALL(renderer, drawTexture(_, _, _, _)).Times(2);

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

    scene.draw(renderer, Size{.width = 1400, .height = 800}, logo);
}
