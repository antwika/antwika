#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/ui/ButtonState.hpp"
#include "antwika/ui/Context.hpp"
#include "antwika/ui/DrawCommand.hpp"
#include "antwika/ui/DrawList.hpp"
#include "antwika/ui/Frame.hpp"
#include "antwika/ui/Hover.hpp"
#include "antwika/ui/HoverPointer.hpp"
#include "antwika/ui/HoverTargets.hpp"
#include "antwika/ui/Interactions.hpp"
#include "antwika/ui/Keyboard.hpp"
#include "antwika/ui/Pointer.hpp"
#include "antwika/ui/Theme.hpp"
#include "antwika/ui/WidgetId.hpp"

using antwika::gfx::Color;
using antwika::gfx::Point;
using antwika::gfx::Size;
using antwika::ui::applyHover;
using antwika::ui::ButtonState;
using antwika::ui::Context;
using antwika::ui::DrawList;
using antwika::ui::FillRect;
using antwika::ui::Frame;
using antwika::ui::HoverPointer;
using antwika::ui::HoverTargets;
using antwika::ui::Interactions;
using antwika::ui::Key;
using antwika::ui::Keyboard;
using antwika::ui::kNoWidget;
using antwika::ui::Pointer;
using antwika::ui::Theme;
using antwika::ui::WidgetId;

namespace
{
    constexpr Color kIdleColor{.red = 40, .green = 50, .blue = 60};
    constexpr Color kHoveredColor{.red = 70, .green = 80, .blue = 90};
    constexpr Color kPressedColor{.red = 15, .green = 25, .blue = 35};

    constexpr WidgetId kLeftWidget{1};
    constexpr WidgetId kRightWidget{2};
    constexpr WidgetId kFieldWidget{3};
    constexpr WidgetId kListWidget{4};

    constexpr Size kCanvasSize{.width = 100, .height = 50};

    constexpr Point kOnTheLeftPoint{.x = 5, .y = 4};
    constexpr Point kOnTheRightPoint{.x = 17, .y = 4};
    constexpr Point kOnNeitherPoint{.x = 80, .y = 40};

    Theme plainTheme()
    {
        return Theme{
            .buttonIdleColor = kIdleColor,
            .buttonHoveredColor = kHoveredColor,
            .buttonPressedColor = kPressedColor,
            .textScale = 1,
            .padding = 0,
            .gap = 0,
            .buttonPadding = 0};
    }

    [[nodiscard]] Frame twoButtons(Pointer pointer = {})
    {
        Context uiContext{kCanvasSize, plainTheme(), pointer};

        {
            const auto actions = uiContext.row();

            uiContext.button("ab", {.widgetId = kLeftWidget});
            uiContext.button("cd", {.widgetId = kRightWidget});
        }

        return uiContext.build();
    }

    [[nodiscard]] Frame everything(Pointer pointer)
    {
        static constexpr std::array<std::string_view, 2> kOptions{
            "one", "two"};
        static constexpr std::string_view kHeld{"ab"};

        Context uiContext{
            kCanvasSize,
            plainTheme(),
            pointer,
            Keyboard{.keys = {Key::Character}, .typedText = "c"},
            kFieldWidget};

        uiContext.textField(
            {.widgetId = kFieldWidget, .text = kHeld, .focused = true});
        uiContext.dropdown({
            .widgetId = kListWidget,
            .optionIdBaseWidget = kLeftWidget,
            .options = kOptions,
            .open = true});

        return uiContext.build();
    }

    [[nodiscard]] Color fillOf(const DrawList &drawList,
        std::size_t commandIndex)
    {
        return std::get<FillRect>(drawList.at(commandIndex)).color;
    }

    [[nodiscard]] HoverPointer at(Point point)
    {
        return HoverPointer{.positionPoint = point};
    }

    [[nodiscard]] std::vector<WidgetId> idsOf(const HoverTargets &targets)
    {
        std::vector<WidgetId> widgetIds;

        for (const auto &target : targets)
        {
            widgetIds.push_back(target.widgetId);
        }

        return widgetIds;
    }
}

TEST(ContextHoverTest, Build_ReportsOneTargetPerInteractiveNamedWidget)
{
    const auto frame = twoButtons();

    EXPECT_EQ(
        (std::vector<WidgetId>{kLeftWidget, kRightWidget}),
        idsOf(frame.hoverTargets));
}

TEST(ContextHoverTest, Build_PlacesEveryTargetWhereTheLayoutPutIt)
{
    const auto frame = twoButtons();

    ASSERT_EQ(2U, frame.hoverTargets.size());

    for (const auto &target : frame.hoverTargets)
    {
        EXPECT_EQ(frame.rects.find(target.widgetId), target.rect);
    }
}

TEST(ContextHoverTest, Build_NamesTheCommandThatFillsEachWidget)
{
    const auto frame = twoButtons();

    ASSERT_EQ(2U, frame.hoverTargets.size());

    for (const auto &target : frame.hoverTargets)
    {
        EXPECT_EQ(target.idleColor, fillOf(frame.drawList, target.command));
    }
}

TEST(ContextHoverTest, Build_ReportsNoTargetForAButtonTheCallerDressed)
{
    Context uiContext{kCanvasSize, plainTheme()};

    uiContext.button(
        "ab",
        {.widgetId = kLeftWidget, .state = ButtonState::Idle});

    EXPECT_TRUE(uiContext.build().hoverTargets.empty());
}

TEST(ContextHoverTest, Build_ReportsNoTargetForAnUnnamedButton)
{
    Context uiContext{kCanvasSize, plainTheme()};

    uiContext.button("ab");

    EXPECT_TRUE(uiContext.build().hoverTargets.empty());
}

TEST(ContextHoverTest, Build_ReportsNoTargetForAFrameWithNoWidgets)
{
    Context uiContext{kCanvasSize, plainTheme()};

    uiContext.label("just words");

    EXPECT_TRUE(uiContext.build().hoverTargets.empty());
}

TEST(ContextHoverTest, ApplyHover_LightsAButtonTheEventStreamNeverSaw)
{
    auto frame = twoButtons();

    ASSERT_EQ(kNoWidget, frame.interactions.hoveredWidget);

    applyHover(frame.drawList, frame.hoverTargets, at(kOnTheRightPoint));

    EXPECT_EQ(
        kIdleColor,
        fillOf(
            frame.drawList,
            frame.hoverTargets.at(0).command));
    EXPECT_EQ(
        kHoveredColor, fillOf(
            frame.drawList,
            frame.hoverTargets.at(1).command));
}

TEST(ContextHoverTest, ApplyHover_PutsOutWhatTheRecordedPointerLeftLit)
{
    auto frame = twoButtons(Pointer{.positionPoint = kOnTheLeftPoint});

    ASSERT_EQ(
        kLeftWidget,
        frame.interactions.hoveredWidget);

    applyHover(
        frame.drawList,
        frame.hoverTargets,
        at(kOnTheRightPoint));

    EXPECT_EQ(
        kIdleColor,
        fillOf(
            frame.drawList,
            frame.hoverTargets.at(0).command));
    EXPECT_EQ(
        kHoveredColor, fillOf(
            frame.drawList,
            frame.hoverTargets.at(1).command));
}

TEST(ContextHoverTest, ApplyHover_LeavesAHeldButtonLookingPressed)
{
    auto frame = twoButtons(
        Pointer{.positionPoint = kOnTheLeftPoint, .down = true});

    ASSERT_TRUE(
        frame.hoverTargets.at(0).held);
    ASSERT_FALSE(
        frame.hoverTargets.at(1).held);

    applyHover(frame.drawList, frame.hoverTargets, at(kOnTheLeftPoint));

    EXPECT_EQ(
        kPressedColor, fillOf(
            frame.drawList,
            frame.hoverTargets.at(0).command));
}

TEST(ContextHoverTest, ApplyHover_DrawsTheSamePictureWithNoHoverPointer)
{
    auto frame = twoButtons(Pointer{.positionPoint = kOnTheLeftPoint});
    const auto beforeDrawList = frame.drawList;

    applyHover(frame.drawList, frame.hoverTargets, HoverPointer{});

    EXPECT_EQ(beforeDrawList, frame.drawList);
}

TEST(ContextHoverTest, ApplyHover_LeavesEveryInteractionExactlyAsItWas)
{
    const auto probe = everything(Pointer{});
    const auto option = probe.rects.find(kLeftWidget);

    ASSERT_TRUE(option.has_value());

    auto frame = everything(Pointer{
        .positionPoint = Point{
            .x = option->originPoint.x + 1, .y = option->originPoint.y + 1},
        .pressed = true});

    const Interactions beforeInteractions = frame.interactions;

    ASSERT_NE(kNoWidget, beforeInteractions.activatedWidget);
    ASSERT_NE(
        kNoWidget,
        beforeInteractions.focusedWidget);
    ASSERT_TRUE(beforeInteractions.edit.has_value());
    ASSERT_TRUE(
        beforeInteractions.chosenChoice.has_value());

    for (const auto point :
         {kOnTheLeftPoint, kOnTheRightPoint, kOnNeitherPoint})
    {
        applyHover(frame.drawList, frame.hoverTargets, at(point));

        EXPECT_EQ(beforeInteractions, frame.interactions);
    }
}

TEST(ContextHoverTest, Build_PutsAnOpenListsOptionsInFrontOfTheBox)
{
    const auto frame = everything(Pointer{});

    EXPECT_EQ(
        (std::vector<WidgetId>{kListWidget, kLeftWidget, kRightWidget}),
        idsOf(frame.hoverTargets));
}
