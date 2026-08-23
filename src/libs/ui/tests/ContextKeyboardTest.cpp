#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <variant>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/ui/Context.hpp"
#include "antwika/ui/DrawCommand.hpp"
#include "antwika/ui/DrawList.hpp"
#include "antwika/ui/Keyboard.hpp"
#include "antwika/ui/Pointer.hpp"
#include "antwika/ui/Sizing.hpp"
#include "antwika/ui/Theme.hpp"
#include "antwika/ui/WidgetId.hpp"

using antwika::gfx::Color;
using antwika::gfx::Point;
using antwika::gfx::Rect;
using antwika::gfx::Size;
using antwika::ui::Context;
using antwika::ui::DrawList;
using antwika::ui::FillRect;
using antwika::ui::getFixedSize;
using antwika::ui::Key;
using antwika::ui::Keyboard;
using antwika::widget::kNoWidget;
using antwika::ui::Pointer;
using antwika::ui::getScaledTheme;
using antwika::ui::Theme;
using antwika::widget::WidgetId;

namespace
{
    constexpr Color kRingColor{.red = 200, .green = 210, .blue = 20};

    constexpr WidgetId kOneWidget{1};
    constexpr WidgetId kTwoWidget{2};

    constexpr Size kCanvasSize{.width = 100, .height = 50};

    constexpr Rect kFirstButtonRect{
        .originPoint = {.x = 0, .y = 0}, .size = {.width = 12, .height = 8}};

    constexpr Point kOnTheButtonPoint{.x = 5, .y = 4};

    Theme getPlainTheme(std::uint32_t thickness)
    {
        return Theme{
            .focusRingColor = kRingColor,
            .textScale = 1,
            .padding = 0,
            .gap = 0,
            .buttonPadding = 0,
            .focusRingThickness = thickness};
    }

    DrawList getRingAround(Rect boxRect, std::uint32_t thickness)
    {
        const auto ringHeight = std::min(thickness, boxRect.size.height);
        const auto ringWidth = std::min(thickness, boxRect.size.width);

        const auto bottom = boxRect.originPoint.y
                            + static_cast<std::int32_t>(
                                boxRect.size.height - ringHeight);
        const auto right = boxRect.originPoint.x
                           + static_cast<std::int32_t>(
                               boxRect.size.width - ringWidth);

        return DrawList{
            FillRect{
                .rect =
                    {.originPoint = boxRect.originPoint,
                     .size = {.width = boxRect.size.width,
                     .height = ringHeight}},
                .color = kRingColor},
            FillRect{
                .rect =
                    {.originPoint = {.x = boxRect.originPoint.x, .y = bottom},
                     .size = {.width = boxRect.size.width,
                     .height = ringHeight}},
                .color = kRingColor},
            FillRect{
                .rect =
                    {.originPoint = boxRect.originPoint,
                     .size = {.width = ringWidth,
                     .height = boxRect.size.height}},
                .color = kRingColor},
            FillRect{
                .rect =
                    {.originPoint = {.x = right, .y = boxRect.originPoint.y},
                     .size = {.width = ringWidth,
                     .height = boxRect.size.height}},
                .color = kRingColor}};
    }

    DrawList tailOf(const DrawList &drawList, std::size_t count)
    {
        return DrawList{drawList.end() - count, drawList.end()};
    }
}

TEST(ContextKeyboardTest, Context_DrawsTheSameThingWithoutAKeyboard)
{
    Context bareContext{kCanvasSize, getPlainTheme(2)};
    bareContext.button("ab", {.widgetId = kOneWidget});

    Context keyedContext{kCanvasSize, getPlainTheme(2), Pointer{}, Keyboard{}};
    keyedContext.button("ab", {.widgetId = kOneWidget});

    const auto expectedFrame = bareContext.build();
    const auto actual = keyedContext.build();

    EXPECT_FALSE(expectedFrame.drawList.empty());
    EXPECT_EQ(expectedFrame.drawList, actual.drawList);
    EXPECT_EQ(expectedFrame.interactions, actual.interactions);
    EXPECT_EQ(kNoWidget, actual.interactions.focusedWidget);
}

TEST(ContextKeyboardTest, Context_TabFocusesTheFirstButton)
{
    Context uiContext{
        kCanvasSize,
        getPlainTheme(2),
        Pointer{},
        Keyboard{.keys = {Key::FocusNext}}};

    uiContext.button("ab", {.widgetId = kOneWidget});
    uiContext.button("cd", {.widgetId = kTwoWidget});

    EXPECT_EQ(kOneWidget, uiContext.build().interactions.focusedWidget);
}

TEST(ContextKeyboardTest, Context_TabCarriesOnFromTheFocusItWasGiven)
{
    Context uiContext{
        kCanvasSize,
        getPlainTheme(2),
        Pointer{},
        Keyboard{.keys = {Key::FocusNext}},
        kOneWidget};

    uiContext.button("ab", {.widgetId = kOneWidget});
    uiContext.button("cd", {.widgetId = kTwoWidget});

    EXPECT_EQ(kTwoWidget, uiContext.build().interactions.focusedWidget);
}

TEST(ContextKeyboardTest, Context_SkipsAButtonNothingCanName)
{
    Context uiContext{
        kCanvasSize,
        getPlainTheme(2),
        Pointer{},
        Keyboard{.keys = {Key::FocusNext}}};

    uiContext.button("ab");
    uiContext.button("cd", {.widgetId = kTwoWidget});

    EXPECT_EQ(kTwoWidget, uiContext.build().interactions.focusedWidget);
}

TEST(ContextKeyboardTest, Context_ActivatesTheFocusedButtonOnEnter)
{
    Context uiContext{
        kCanvasSize,
        getPlainTheme(2),
        Pointer{},
        Keyboard{.keys = {Key::Activate}},
        kTwoWidget};

    uiContext.button("ab", {.widgetId = kOneWidget});
    uiContext.button("cd", {.widgetId = kTwoWidget});

    EXPECT_EQ(kTwoWidget, uiContext.build().interactions.activatedWidget);
}

TEST(ContextKeyboardTest, Context_HandsFocusOnToTheNextFrame)
{
    Context firstContext{
        kCanvasSize,
        getPlainTheme(2),
        Pointer{},
        Keyboard{.keys = {Key::FocusNext, Key::FocusNext}}};

    firstContext.button("ab", {.widgetId = kOneWidget});
    firstContext.button("cd", {.widgetId = kTwoWidget});

    const auto focus = firstContext.build().interactions.focusedWidget;

    Context secondContext{
        kCanvasSize,
        getPlainTheme(2),
        Pointer{},
        Keyboard{.keys = {Key::Activate}},
        focus};

    secondContext.button("ab", {.widgetId = kOneWidget});
    secondContext.button("cd", {.widgetId = kTwoWidget});

    EXPECT_EQ(kTwoWidget, focus);
    EXPECT_EQ(kTwoWidget, secondContext.build().interactions.activatedWidget);
}

TEST(ContextKeyboardTest, Context_DrawsFourBarsRoundTheFocusedButton)
{
    Context uiContext{kCanvasSize, getPlainTheme(
        3), Pointer{}, Keyboard{}, kOneWidget};

    uiContext.button("ab", {.widgetId = kOneWidget});
    uiContext.button("cd", {.widgetId = kTwoWidget});

    const auto commands = uiContext.build().drawList;

    EXPECT_EQ(getRingAround(kFirstButtonRect, 3), tailOf(commands, 4));
}

TEST(ContextKeyboardTest, Context_ClampsTheRingToASmallerWidget)
{
    Context uiContext{kCanvasSize, getPlainTheme(
        40), Pointer{}, Keyboard{}, kOneWidget};

    uiContext.button(
        "ab",
        {.widgetId = kOneWidget, .widthSizing = getFixedSize(4)});

    const auto commands = uiContext.build().drawList;
    const auto box = std::get<FillRect>(commands.at(0)).rect;

    EXPECT_EQ(4U, box.size.width);
    EXPECT_EQ(getRingAround(box, 40), tailOf(commands, 4));
}

TEST(ContextKeyboardTest, Context_DrawsNoRingWhenTheThemeHasNoThickness)
{
    Context bareContext{kCanvasSize, getPlainTheme(0)};
    bareContext.button("ab", {.widgetId = kOneWidget});

    Context focusedContext{
        kCanvasSize,
        getPlainTheme(0),
        Pointer{},
        Keyboard{},
        kOneWidget};
    focusedContext.button("ab", {.widgetId = kOneWidget});

    const auto expectedFrame = focusedContext.build();

    EXPECT_FALSE(expectedFrame.drawList.empty());
    EXPECT_EQ(bareContext.build().drawList, expectedFrame.drawList);
    EXPECT_EQ(kOneWidget, expectedFrame.interactions.focusedWidget);
}

TEST(ContextKeyboardTest, Context_MovesTheRingToAClickedButton)
{
    Context uiContext{
        kCanvasSize,
        getPlainTheme(3),
        Pointer{.positionPoint = kOnTheButtonPoint, .pressed = true},
        Keyboard{},
        kTwoWidget};

    uiContext.button("ab", {.widgetId = kOneWidget});
    uiContext.button("cd", {.widgetId = kTwoWidget});

    const auto frame = uiContext.build();

    EXPECT_EQ(kOneWidget, frame.interactions.focusedWidget);
    EXPECT_EQ(getRingAround(kFirstButtonRect, 3), tailOf(frame.drawList, 4));
}

TEST(ContextKeyboardTest, Theme_DefaultsToAYellowRingOnePixelThick)
{
    constexpr Theme theme{};

    EXPECT_EQ(1U, theme.focusRingThickness);
    EXPECT_EQ(
        (Color{.red = 244, .green = 208, .blue = 63}), theme.focusRingColor);
}

TEST(ContextKeyboardTest, ScaledTheme_MultipliesTheRingThickness)
{
    constexpr Theme baseTheme{};

    const auto theme = getScaledTheme(baseTheme, 3);

    EXPECT_EQ(3U, theme.focusRingThickness);
    EXPECT_EQ(baseTheme.focusRingColor, theme.focusRingColor);
}
