#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/TextLayout.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/gfx/mocks/MockTexture.hpp>
#include <antwika/ui/DrawCommand.hpp>
#include <antwika/ui/DrawList.hpp>
#include <antwika/ui/Pointer.hpp>
#include <antwika/ui/WidgetId.hpp>

#include "antwika/gfx_demo/DemoScene.hpp"

using antwika::gfx::Color;
using antwika::gfx::Point;
using antwika::gfx::Rect;
using antwika::gfx::Size;
using antwika::gfx::textSize;
using antwika::gfx::mocks::MockRenderer;
using antwika::gfx::mocks::MockTexture;
using antwika::gfx_demo::DemoScene;
namespace widgets = antwika::gfx_demo::widgets;
using antwika::ui::DrawList;
using antwika::ui::DrawText;
using antwika::ui::FillRect;
using antwika::ui::Pointer;
using antwika::ui::WidgetId;
using ::testing::_;
using ::testing::AnyNumber;
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

    constexpr Size kCanvas{.width = 700, .height = 400};

    [[nodiscard]] std::vector<std::string> textsOf(
        const DrawList &commands)
    {
        std::vector<std::string> texts;

        for (const auto &command : commands)
        {
            if (const auto *text = std::get_if<DrawText>(&command))
            {
                texts.push_back(text->text);
            }
        }

        return texts;
    }

    [[nodiscard]] std::vector<Color> fillsOf(const DrawList &commands)
    {
        std::vector<Color> colors;

        for (const auto &command : commands)
        {
            if (const auto *fill = std::get_if<FillRect>(&command))
            {
                colors.push_back(fill->color);
            }
        }

        return colors;
    }

    // Where a widget ended up is the layout's business.
    // So a test looks for a pixel that hits the one it means.
    // Stepping by four cannot miss a button several glyphs tall.
    [[nodiscard]] Pointer pointerOn(WidgetId id)
    {
        const DemoScene scene;

        for (std::int32_t y = 0; y < static_cast<std::int32_t>(
                 kCanvas.height); y += 4)
        {
            for (std::int32_t x = 0; x < static_cast<std::int32_t>(
                     kCanvas.width); x += 4)
            {
                const Pointer pointer{.position = Point{.x = x, .y = y}};

                if (scene.describe(kCanvas, pointer).interactions.hovered
                    == id)
                {
                    return pointer;
                }
            }
        }

        return Pointer{};
    }

    [[nodiscard]] std::optional<DrawText> firstTextOf(
        const DrawList &commands)
    {
        for (const auto &command : commands)
        {
            if (const auto *text = std::get_if<DrawText>(&command))
            {
                return *text;
            }
        }

        return std::nullopt;
    }
} // namespace

// Each test pins the one thing it is about.
// Everything else the scene draws is allowed and ignored.
// That keeps the UI panel's own drawing out of the texture tests.
class DemoSceneTest : public ::testing::Test
{
protected:
    DemoSceneTest()
    {
        ON_CALL(logo, size()).WillByDefault(Return(kLogoSize));

        EXPECT_CALL(renderer, drawRect(_, _)).Times(AnyNumber());
        EXPECT_CALL(renderer, drawText(_, _, _, _)).Times(AnyNumber());
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

    scene.draw(renderer, kCanvas, logo, scene.describe(kCanvas).commands);
}

TEST_F(DemoSceneTest, Draw_BlitsTheWholeLogoUntintedAboveTheBars)
{
    EXPECT_CALL(renderer, clear(_));
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

    scene.draw(renderer, kCanvas, logo, scene.describe(kCanvas).commands);
}

// The badge is an eighth of the height.
// So a tall narrow window makes it wider than the canvas.
// The centring subtraction is unsigned and used to wrap there.
TEST_F(DemoSceneTest, Draw_KeepsTheBadgeOnTheCanvasWhenItIsTooWide)
{
    EXPECT_CALL(renderer, clear(_));
    EXPECT_CALL(renderer, drawTexture(_, _, _, kWarmTint));

    // 900/8 is a 112px badge across a canvas 100px wide.
    EXPECT_CALL(
        renderer,
        drawTexture(
            Ref(logo),
            _,
            Rect{
                .origin = {.x = 0, .y = 56},
                .size = {.width = 112, .height = 112}},
            kUntinted));

    scene.draw(
        renderer, Size{.width = 100, .height = 900}, logo, DrawList{});
}

TEST_F(DemoSceneTest, Draw_BlitsTheLogosLeftHalfTintedBelowTheBars)
{
    EXPECT_CALL(renderer, clear(_));
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

    scene.draw(renderer, kCanvas, logo, scene.describe(kCanvas).commands);
}

TEST_F(DemoSceneTest, Draw_AsksTheTextureForItsSizeRatherThanAssuming)
{
    // A different logo must change the source rectangles.
    // The scene has no idea what it was handed until it asks.
    EXPECT_CALL(logo, size())
        .WillRepeatedly(Return(Size{.width = 20, .height = 10}));

    EXPECT_CALL(renderer, clear(_));

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

    scene.draw(renderer, kCanvas, logo, scene.describe(kCanvas).commands);
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

    scene.draw(
        renderer,
        Size{.width = 1400, .height = 800},
        logo,
        DrawList{});
}

TEST_F(DemoSceneTest, Describe_DrawsEveryLabelAndButton)
{
    EXPECT_THAT(
        textsOf(scene.describe(kCanvas).commands),
        ::testing::IsSupersetOf(
            {"Antwika UI",
             "layouts",
             "buttons",
             "text",
             "clicks 0",
             "reset",
             "count"}));
}

// Nothing here can clip.
// So the layout is what has to keep every widget inside the window.
TEST_F(DemoSceneTest, Describe_KeepsEveryWidgetInsideTheCanvas)
{
    const auto right = static_cast<std::int32_t>(kCanvas.width);
    const auto bottom = static_cast<std::int32_t>(kCanvas.height);

    for (const auto &command : scene.describe(kCanvas).commands)
    {
        if (const auto *fill = std::get_if<FillRect>(&command))
        {
            EXPECT_GE(fill->rect.origin.x, 0);
            EXPECT_GE(fill->rect.origin.y, 0);
            EXPECT_LE(
                fill->rect.origin.x
                    + static_cast<std::int32_t>(fill->rect.size.width),
                right);
            EXPECT_LE(
                fill->rect.origin.y
                    + static_cast<std::int32_t>(fill->rect.size.height),
                bottom);

            continue;
        }

        const auto &text = std::get<DrawText>(command);
        const auto extent = textSize(text.text, text.scale);

        EXPECT_GE(text.origin.x, 0);
        EXPECT_GE(text.origin.y, 0);
        EXPECT_LE(
            text.origin.x + static_cast<std::int32_t>(extent.width),
            right);
        EXPECT_LE(
            text.origin.y + static_cast<std::int32_t>(extent.height),
            bottom);
    }
}

// A hovered button is meant to look different from an idle one.
// Nothing tells it which it is: it works that out from the pointer.
TEST_F(DemoSceneTest, Describe_ShowsAHoveredButtonDifferently)
{
    const auto resting = fillsOf(scene.describe(kCanvas).commands);
    const auto over = fillsOf(
        scene.describe(kCanvas, pointerOn(widgets::kCount)).commands);

    ASSERT_EQ(resting.size(), over.size());
    EXPECT_NE(resting, over);
}

TEST_F(DemoSceneTest, Describe_ReportsWhichButtonAPressLandedOn)
{
    auto pointer = pointerOn(widgets::kCount);
    pointer.pressed = true;

    const auto frame = scene.describe(kCanvas, pointer);

    EXPECT_EQ(widgets::kCount, frame.interactions.activated);
}

TEST_F(DemoSceneTest, Describe_ReportsNoActivationForAPressOffThePanel)
{
    const Pointer pointer{
        .position = Point{
            .x = static_cast<std::int32_t>(kCanvas.width) - 1,
            .y = static_cast<std::int32_t>(kCanvas.height) - 1},
        .down = true,
        .pressed = true};

    const auto frame = scene.describe(kCanvas, pointer);

    EXPECT_EQ(antwika::ui::kNoWidget, frame.interactions.activated);
    EXPECT_FALSE(frame.interactions.pointerOverUi);
}

TEST_F(DemoSceneTest, Describe_CountsUpWhatItIsToldToShow)
{
    EXPECT_THAT(
        textsOf(scene.describe(kCanvas, Pointer{}, 12).commands),
        ::testing::Contains("clicks 12"));
}

// The panel takes a third of the width.
// So it cannot cover the logo blitted across the middle of the canvas.
TEST_F(DemoSceneTest, Describe_LeavesTheRestOfTheCanvasAlone)
{
    const auto third = static_cast<std::int32_t>(kCanvas.width / 3);

    for (const auto &command : scene.describe(kCanvas).commands)
    {
        const auto *fill = std::get_if<FillRect>(&command);

        if (fill == nullptr)
        {
            continue;
        }

        EXPECT_LE(
            fill->rect.origin.x
                + static_cast<std::int32_t>(fill->rect.size.width),
            third);
    }
}

TEST_F(DemoSceneTest, Describe_ScalesTheTextToTheCanvas)
{
    const auto small = firstTextOf(
        scene.describe(Size{.width = 320, .height = 100}).commands);
    const auto large = firstTextOf(
        scene.describe(Size{.width = 1280, .height = 720}).commands);

    ASSERT_TRUE(small.has_value());
    ASSERT_TRUE(large.has_value());

    EXPECT_EQ(1U, small->scale);
    EXPECT_EQ(3U, large->scale);
}

// A canvas with no room for a single glyph gets no text at all.
// Text is never drawn outside the canvas instead.
TEST_F(DemoSceneTest, Describe_DrawsNoTextOnACanvasTooSmallForAny)
{
    const auto picture =
        scene.describe(Size{.width = 1, .height = 1}).commands;

    EXPECT_TRUE(textsOf(picture).empty());
}

// The panel goes on last, so it reads as being in front of the scene.
TEST_F(DemoSceneTest, Draw_PaintsThePanelAfterTheTextures)
{
    const InSequence sequence;

    EXPECT_CALL(renderer, clear(_));
    EXPECT_CALL(renderer, drawTexture(_, _, _, _)).Times(2);
    EXPECT_CALL(renderer, drawText(_, _, _, _)).Times(AnyNumber());

    scene.draw(renderer, kCanvas, logo, scene.describe(kCanvas).commands);
}
