#include <gtest/gtest.h>

#include <cstdint>
#include <optional>
#include <variant>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/ui/Axis.hpp"
#include "antwika/ui/Context.hpp"
#include "antwika/ui/DrawCommand.hpp"
#include "antwika/ui/Frame.hpp"
#include "antwika/ui/Pointer.hpp"
#include "antwika/ui/Sizing.hpp"
#include "antwika/ui/Theme.hpp"
#include "antwika/ui/WidgetId.hpp"
#include "antwika/ui/WidgetRects.hpp"

#include "Layout.hpp"
#include "LayoutTree.hpp"
#include "Node.hpp"

using antwika::gfx::Color;
using antwika::gfx::Point;
using antwika::gfx::Rect;
using antwika::gfx::Size;
using antwika::ui::Axis;
using antwika::ui::Context;
using antwika::ui::FillRect;
using antwika::ui::fixedSize;
using antwika::ui::Frame;
using antwika::ui::kFitSizing;
using antwika::ui::kGrowSizing;
using antwika::ui::kNoWidget;
using antwika::ui::Sizing;
using antwika::ui::Theme;
using antwika::ui::WidgetId;
using antwika::ui::WidgetRect;
using antwika::ui::WidgetRects;
using antwika::ui::detail::LayoutTree;
using antwika::ui::detail::layout;
using antwika::ui::detail::Node;

namespace
{
    constexpr WidgetId kFirstWidget{1};
    constexpr WidgetId kSecondWidget{2};
    constexpr WidgetId kThirdWidget{3};
    constexpr WidgetId kAbsentWidget{99};

    constexpr Color kPanelColor{.red = 10, .green = 20, .blue = 30};
    constexpr Color kAccentColor{.red = 90, .green = 40, .blue = 40};
    constexpr Color kInkColor{.red = 200, .green = 210, .blue = 220};

    Theme plainTheme()
    {
        return Theme{
            .panelColor = kPanelColor,
            .textColor = kInkColor,
            .textScale = 1,
            .padding = 0,
            .gap = 0,
            .buttonPadding = 0};
    }

    Node container(
        Axis axis, Sizing widthSizing, Sizing heightSizing, WidgetId widget)
    {
        return Node{
            .axis = axis,
            .widthSizing = widthSizing,
            .heightSizing = heightSizing,
            .widgetId = widget};
    }

    std::optional<Rect> filled(const Frame &frame, Color color)
    {
        for (const auto &command : frame.drawList)
        {
            const auto *fill = std::get_if<FillRect>(&command);

            if (fill != nullptr && fill->color == color)
            {
                return fill->rect;
            }
        }

        return {};
    }
}

TEST(WidgetRectsTest, Find_AnswersNothingWhenNothingWasNamed)
{
    const WidgetRects rects;

    EXPECT_FALSE(rects.find(kFirstWidget).has_value());
}

TEST(WidgetRectsTest, Find_AnswersTheRectangleItWasGiven)
{
    const Rect boxRect{
        .originPoint = {.x = 3, .y = 4}, .size = {.width = 5, .height = 6}};
    const WidgetRects rects{.widgetRects = {WidgetRect{
                                .widgetId = kFirstWidget, .rect = boxRect}}};

    EXPECT_EQ(boxRect, rects.find(kFirstWidget));
}

TEST(WidgetRectsTest, Find_AnswersNothingForAnIdNoNodeCarried)
{
    const WidgetRects rects{
        .widgetRects = {WidgetRect{.widgetId = kFirstWidget, .rect = {}}}};

    EXPECT_FALSE(rects.find(kAbsentWidget).has_value());
}

TEST(WidgetRectsTest, OperatorEquals_ComparesTheEntriesAndTheirOrder)
{
    const Rect boxRect{
        .originPoint = {.x = 1, .y = 1}, .size = {.width = 2, .height = 2}};

    const WidgetRects oneRects{
        .widgetRects = {
            WidgetRect{.widgetId = kFirstWidget, .rect = {}},
            WidgetRect{.widgetId = kSecondWidget, .rect = {}}}};
    const WidgetRects sameRects{
        .widgetRects = {
            WidgetRect{.widgetId = kFirstWidget, .rect = {}},
            WidgetRect{.widgetId = kSecondWidget, .rect = {}}}};
    const WidgetRects swappedRects{
        .widgetRects = {
            WidgetRect{.widgetId = kSecondWidget, .rect = {}},
            WidgetRect{.widgetId = kFirstWidget, .rect = {}}}};
    const WidgetRects movedRects{
        .widgetRects = {
            WidgetRect{.widgetId = kFirstWidget, .rect = boxRect},
            WidgetRect{.widgetId = kSecondWidget, .rect = {}}}};

    EXPECT_TRUE(oneRects == sameRects);
    EXPECT_FALSE(oneRects == swappedRects);

    EXPECT_FALSE(oneRects == movedRects);
}

TEST(WidgetRectsTest, Layout_CollectsNothingWhenNoMappingIsAskedFor)
{
    LayoutTree tree{container(Axis::Row, kGrowSizing, kGrowSizing, kNoWidget)};

    const auto child =
        tree.add(container(Axis::Row, kGrowSizing, kGrowSizing, kFirstWidget));

    layout(tree, Size{.width = 40, .height = 10});

    EXPECT_EQ(40U, tree.node(child).arrangedRect.size.width);
}

TEST(WidgetRectsTest, Layout_LeavesAnUnnamedNodeOutOfTheMapping)
{
    LayoutTree tree{container(Axis::Row, kGrowSizing, kGrowSizing, kNoWidget)};

    tree.add(container(Axis::Row, kGrowSizing, kGrowSizing, kNoWidget));

    WidgetRects rects;

    layout(tree, Size{.width = 40, .height = 10}, &rects);

    EXPECT_TRUE(rects.widgetRects.empty());
}

TEST(WidgetRectsTest, Layout_ReportsTheRectangleTheNodeWasArrangedInto)
{
    LayoutTree tree{container(Axis::Row, kGrowSizing, kGrowSizing, kNoWidget)};

    const auto first =
        tree.add(container(Axis::Row, kGrowSizing, kGrowSizing, kFirstWidget));
    const auto second =
        tree.add(container(Axis::Row, kGrowSizing, kGrowSizing, kSecondWidget));

    WidgetRects rects;

    layout(tree, Size{.width = 40, .height = 10}, &rects);

    EXPECT_EQ(tree.node(first).arrangedRect, rects.find(kFirstWidget));
    EXPECT_EQ(tree.node(second).arrangedRect, rects.find(kSecondWidget));
}

TEST(WidgetRectsTest, Layout_ReportsTheShrunkRectangleAndNotTheAskedFor)
{
    LayoutTree tree{container(Axis::Row, kGrowSizing, kGrowSizing, kNoWidget)};

    tree.add(container(Axis::Row, fixedSize(40), kGrowSizing, kFirstWidget));
    tree.add(container(Axis::Row, fixedSize(40), kGrowSizing, kSecondWidget));
    tree.add(container(Axis::Row, fixedSize(40), kGrowSizing, kThirdWidget));

    WidgetRects rects;

    layout(tree, Size{.width = 50, .height = 10}, &rects);

    EXPECT_EQ(
        (Rect{
            .originPoint = {.x = 0, .y = 0},
            .size = {.width = 17, .height = 10}}),
        rects.find(kFirstWidget));
    EXPECT_EQ(
        (Rect{
            .originPoint = {.x = 17, .y = 0},
            .size = {.width = 17, .height = 10}}),
        rects.find(kSecondWidget));
    EXPECT_EQ(
        (Rect{
            .originPoint = {.x = 34, .y = 0},
            .size = {.width = 16, .height = 10}}),
        rects.find(kThirdWidget));
}

TEST(WidgetRectsTest, Layout_KeepsTheLastDeclarationOfARepeatedId)
{
    LayoutTree tree{container(Axis::Row, kGrowSizing, kGrowSizing, kNoWidget)};

    tree.add(container(Axis::Row, fixedSize(10), kGrowSizing, kFirstWidget));
    const auto second =
        tree.add(
            container(Axis::Row, fixedSize(10), kGrowSizing, kFirstWidget));

    WidgetRects rects;

    layout(tree, Size{.width = 40, .height = 10}, &rects);

    EXPECT_EQ(1U, rects.widgetRects.size());
    EXPECT_EQ(tree.node(second).arrangedRect, rects.find(kFirstWidget));
    EXPECT_EQ(10, tree.node(second).arrangedRect.originPoint.x);
}

TEST(WidgetRectsTest, Frame_ReportsNothingForAUiThatNamesNothing)
{
    Context uiContext{Size{.width = 40, .height = 20}, plainTheme()};

    {
        const auto body = uiContext.column();

        uiContext.label("ab");
    }

    EXPECT_TRUE(uiContext.build().rects.widgetRects.empty());
}

TEST(WidgetRectsTest, Frame_ReportsTheRectangleAContainerWasFilledWith)
{
    Context uiContext{Size{.width = 40, .height = 20}, plainTheme()};

    {
        const auto body = uiContext.row({
            .heightSizing = fixedSize(8),
            .backgroundColor = kAccentColor,
            .widgetId = kFirstWidget});

        uiContext.label("ab");
    }

    const auto frame = uiContext.build();

    EXPECT_EQ(filled(frame, kAccentColor), frame.rects.find(kFirstWidget));
    EXPECT_EQ(
        (Rect{
            .originPoint = {.x = 0, .y = 0},
            .size = {.width = 40, .height = 8}}),
        frame.rects.find(kFirstWidget));
}

TEST(WidgetRectsTest, Frame_ReportsAButtonsRectangleToo)
{
    Context uiContext{Size{.width = 40, .height = 20}, plainTheme()};

    uiContext.button(
        "ab",
        {.widgetId = kSecondWidget, .widthSizing = fixedSize(20)});

    const auto frame = uiContext.build();

    EXPECT_EQ(
        (Rect{
            .originPoint = {.x = 0, .y = 0},
            .size = {.width = 20, .height = 8}}),
        frame.rects.find(kSecondWidget));
}

TEST(WidgetRectsTest, Frame_KeepsANestedRectangleInsideItsParents)
{
    Context uiContext{Size{.width = 60, .height = 30}, plainTheme()};

    {
        const auto outer = uiContext.column({
            .widthSizing = fixedSize(40),
            .heightSizing = fixedSize(20),
            .padding = 4U,
            .widgetId = kFirstWidget});

        {
            const auto inner = uiContext.row({
                .heightSizing = fixedSize(6),
                .widgetId = kSecondWidget});

            uiContext.label("ab");
        }
    }

    const auto frame = uiContext.build();
    const auto parent = frame.rects.find(kFirstWidget);
    const auto child = frame.rects.find(kSecondWidget);

    ASSERT_TRUE(parent.has_value());
    ASSERT_TRUE(child.has_value());

    EXPECT_GE(child->originPoint.x, parent->originPoint.x);
    EXPECT_GE(child->originPoint.y, parent->originPoint.y);
    EXPECT_LE(
        child->originPoint.x + static_cast<std::int32_t>(child->size.width),
        parent->originPoint.x
            + static_cast<std::int32_t>(parent->size.width));
    EXPECT_LE(
        child->originPoint.y + static_cast<std::int32_t>(child->size.height),
        parent->originPoint.y
            + static_cast<std::int32_t>(parent->size.height));
}

TEST(WidgetRectsTest, Frame_HoversANamedContainerLikeAnyOtherWidget)
{
    Context uiContext{
        Size{.width = 40, .height = 20},
        plainTheme(),
        {.positionPoint = Point{.x = 5, .y = 2}}};

    {
        const auto body = uiContext.row({
            .heightSizing = fixedSize(8),
            .backgroundColor = kAccentColor,
            .widgetId = kFirstWidget});

        uiContext.label("ab");
    }

    EXPECT_EQ(kFirstWidget, uiContext.build().interactions.hoveredWidget);
}
