#include <gtest/gtest.h>

#include <cstdint>
#include <variant>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/ui/ButtonState.hpp"
#include "antwika/ui/Context.hpp"
#include "antwika/ui/DrawCommand.hpp"
#include "antwika/ui/DrawList.hpp"
#include "antwika/ui/Pointer.hpp"
#include "antwika/ui/Theme.hpp"
#include "antwika/ui/WidgetId.hpp"

using antwika::gfx::Color;
using antwika::gfx::Point;
using antwika::gfx::Size;
using antwika::ui::ButtonState;
using antwika::ui::Context;
using antwika::ui::DrawList;
using antwika::ui::FillRect;
using antwika::ui::kNoWidget;
using antwika::ui::Pointer;
using antwika::ui::Theme;
using antwika::ui::WidgetId;

namespace
{
    constexpr Color kIdleColor{.red = 40, .green = 50, .blue = 60};
    constexpr Color kHoveredColor{.red = 70, .green = 80, .blue = 90};
    constexpr Color kPressedColor{.red = 15, .green = 25, .blue = 35};
    constexpr Color kPanelColor{.red = 10, .green = 20, .blue = 30};

    constexpr WidgetId kOkWidget{1};

    constexpr Size kCanvasSize{.width = 100, .height = 50};

    constexpr Point kOnTheButtonPoint{.x = 5, .y = 4};
    constexpr Point kOffTheButtonPoint{.x = 50, .y = 40};

    Theme plainTheme()
    {
        return Theme{
            .panelColor = kPanelColor,
            .buttonIdleColor = kIdleColor,
            .buttonHoveredColor = kHoveredColor,
            .buttonPressedColor = kPressedColor,
            .textScale = 1,
            .padding = 0,
            .gap = 0,
            .buttonPadding = 0};
    }

    [[nodiscard]] Color fillColorOf(const DrawList &drawList)
    {
        return std::get<FillRect>(drawList.at(0)).color;
    }
}

TEST(ContextInteractionTest, Button_IsHoveredWhenThePointerIsOverIt)
{
    Context uiContext{
        kCanvasSize,
        plainTheme(),
        Pointer{.positionPoint = kOnTheButtonPoint}};

    uiContext.button("ab", {.widgetId = kOkWidget});

    const auto frame = uiContext.build();

    EXPECT_EQ(kOkWidget, frame.interactions.hoveredWidget);
    EXPECT_EQ(kHoveredColor, fillColorOf(frame.drawList));
}

TEST(ContextInteractionTest, Button_LooksPressedWhileItIsHeld)
{
    Context uiContext{
        kCanvasSize,
        plainTheme(),
        Pointer{.positionPoint = kOnTheButtonPoint, .down = true}};

    uiContext.button("ab", {.widgetId = kOkWidget});

    EXPECT_EQ(kPressedColor, fillColorOf(uiContext.build().drawList));
}

TEST(ContextInteractionTest, Button_ActivatesOnThePress)
{
    Context uiContext{
        kCanvasSize,
        plainTheme(),
        Pointer{.positionPoint = kOnTheButtonPoint,
            .down = true,
            .pressed = true}};

    uiContext.button("ab", {.widgetId = kOkWidget});

    EXPECT_EQ(kOkWidget, uiContext.build().interactions.activatedWidget);
}

TEST(ContextInteractionTest, Button_IgnoresAPressLandingElsewhere)
{
    Context uiContext{
        kCanvasSize,
        plainTheme(),
        Pointer{.positionPoint = kOffTheButtonPoint,
            .down = true,
            .pressed = true}};

    uiContext.button("ab", {.widgetId = kOkWidget});

    const auto frame = uiContext.build();

    EXPECT_EQ(kNoWidget, frame.interactions.activatedWidget);
    EXPECT_EQ(kIdleColor, fillColorOf(frame.drawList));
}

TEST(ContextInteractionTest, Button_KeepsAnAppearanceItWasToldToHave)
{
    Context uiContext{
        kCanvasSize,
        plainTheme(),
        Pointer{.positionPoint = kOnTheButtonPoint}};

    uiContext.button("ab", {.widgetId = kOkWidget, .state = ButtonState::Idle});

    const auto frame = uiContext.build();

    EXPECT_EQ(kIdleColor, fillColorOf(frame.drawList));
    EXPECT_EQ(kOkWidget, frame.interactions.hoveredWidget);
}

TEST(ContextInteractionTest, Button_IsNotHoveredWhenItWasNeverNamed)
{
    Context uiContext{
        kCanvasSize,
        plainTheme(),
        Pointer{.positionPoint = kOnTheButtonPoint}};

    uiContext.button("ab");

    const auto frame = uiContext.build();

    EXPECT_EQ(kNoWidget, frame.interactions.hoveredWidget);
    EXPECT_EQ(kIdleColor, fillColorOf(frame.drawList));
}

TEST(ContextInteractionTest, Build_ReportsThePointerOverAPanel)
{
    Context uiContext{
        kCanvasSize,
        plainTheme(),
        Pointer{.positionPoint = kOffTheButtonPoint}};

    {
        const auto screen = uiContext.panel(
            {.heightSizing = antwika::ui::kGrowSizing});

        uiContext.label("ab");
    }

    EXPECT_TRUE(uiContext.build().interactions.pointerOverUi);
}

TEST(ContextInteractionTest, Build_ReportsNothingWithoutAPointer)
{
    Context uiContext{kCanvasSize, plainTheme()};

    uiContext.button("ab", {.widgetId = kOkWidget});

    const auto frame = uiContext.build();

    EXPECT_EQ(kNoWidget, frame.interactions.hoveredWidget);
    EXPECT_FALSE(frame.interactions.pointerOverUi);
    EXPECT_EQ(kIdleColor, fillColorOf(frame.drawList));
}
