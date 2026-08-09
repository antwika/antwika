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
using antwika::ui::kFit;
using antwika::ui::kGrow;
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
    constexpr WidgetId kFirst{1};
    constexpr WidgetId kSecond{2};
    constexpr WidgetId kThird{3};
    constexpr WidgetId kAbsent{99};

    constexpr Color kPanel{.red = 10, .green = 20, .blue = 30};
    constexpr Color kAccent{.red = 90, .green = 40, .blue = 40};
    constexpr Color kInk{.red = 200, .green = 210, .blue = 220};

    Theme plainTheme()
    {
        return Theme{
            .panel = kPanel,
            .text = kInk,
            .textScale = 1,
            .padding = 0,
            .gap = 0,
            .buttonPadding = 0};
    }

    Node container(Axis axis, Sizing width, Sizing height, WidgetId id)
    {
        return Node{
            .axis = axis, .width = width, .height = height, .id = id};
    }

    std::optional<Rect> filled(const Frame &frame, Color color)
    {
        for (const auto &command : frame.commands)
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

    EXPECT_FALSE(rects.find(kFirst).has_value());
}

TEST(WidgetRectsTest, Find_AnswersTheRectangleItWasGiven)
{
    const Rect box{
        .origin = {.x = 3, .y = 4}, .size = {.width = 5, .height = 6}};
    const WidgetRects rects{.entries = {WidgetRect{
                                .id = kFirst, .rect = box}}};

    EXPECT_EQ(box, rects.find(kFirst));
}

TEST(WidgetRectsTest, Find_AnswersNothingForAnIdNoNodeCarried)
{
    const WidgetRects rects{
        .entries = {WidgetRect{.id = kFirst, .rect = {}}}};

    EXPECT_FALSE(rects.find(kAbsent).has_value());
}

TEST(WidgetRectsTest, OperatorEquals_ComparesTheEntriesAndTheirOrder)
{
    const Rect box{
        .origin = {.x = 1, .y = 1}, .size = {.width = 2, .height = 2}};

    const WidgetRects one{
        .entries = {
            WidgetRect{.id = kFirst, .rect = {}},
            WidgetRect{.id = kSecond, .rect = {}}}};
    const WidgetRects same{
        .entries = {
            WidgetRect{.id = kFirst, .rect = {}},
            WidgetRect{.id = kSecond, .rect = {}}}};
    const WidgetRects swapped{
        .entries = {
            WidgetRect{.id = kSecond, .rect = {}},
            WidgetRect{.id = kFirst, .rect = {}}}};
    const WidgetRects moved{
        .entries = {
            WidgetRect{.id = kFirst, .rect = box},
            WidgetRect{.id = kSecond, .rect = {}}}};

    EXPECT_TRUE(one == same);
    EXPECT_FALSE(one == swapped);

    EXPECT_FALSE(one == moved);
}

TEST(WidgetRectsTest, Layout_CollectsNothingWhenNoMappingIsAskedFor)
{
    LayoutTree tree{container(Axis::Row, kGrow, kGrow, kNoWidget)};

    const auto child =
        tree.add(container(Axis::Row, kGrow, kGrow, kFirst));

    layout(tree, Size{.width = 40, .height = 10});

    EXPECT_EQ(40U, tree.node(child).arranged.size.width);
}

TEST(WidgetRectsTest, Layout_LeavesAnUnnamedNodeOutOfTheMapping)
{
    LayoutTree tree{container(Axis::Row, kGrow, kGrow, kNoWidget)};

    tree.add(container(Axis::Row, kGrow, kGrow, kNoWidget));

    WidgetRects rects;

    layout(tree, Size{.width = 40, .height = 10}, &rects);

    EXPECT_TRUE(rects.entries.empty());
}

TEST(WidgetRectsTest, Layout_ReportsTheRectangleTheNodeWasArrangedInto)
{
    LayoutTree tree{container(Axis::Row, kGrow, kGrow, kNoWidget)};

    const auto first =
        tree.add(container(Axis::Row, kGrow, kGrow, kFirst));
    const auto second =
        tree.add(container(Axis::Row, kGrow, kGrow, kSecond));

    WidgetRects rects;

    layout(tree, Size{.width = 40, .height = 10}, &rects);

    EXPECT_EQ(tree.node(first).arranged, rects.find(kFirst));
    EXPECT_EQ(tree.node(second).arranged, rects.find(kSecond));
}

TEST(WidgetRectsTest, Layout_ReportsTheShrunkRectangleAndNotTheAskedFor)
{
    LayoutTree tree{container(Axis::Row, kGrow, kGrow, kNoWidget)};

    tree.add(container(Axis::Row, fixedSize(40), kGrow, kFirst));
    tree.add(container(Axis::Row, fixedSize(40), kGrow, kSecond));
    tree.add(container(Axis::Row, fixedSize(40), kGrow, kThird));

    WidgetRects rects;

    layout(tree, Size{.width = 50, .height = 10}, &rects);

    EXPECT_EQ(
        (Rect{
            .origin = {.x = 0, .y = 0},
            .size = {.width = 17, .height = 10}}),
        rects.find(kFirst));
    EXPECT_EQ(
        (Rect{
            .origin = {.x = 17, .y = 0},
            .size = {.width = 17, .height = 10}}),
        rects.find(kSecond));
    EXPECT_EQ(
        (Rect{
            .origin = {.x = 34, .y = 0},
            .size = {.width = 16, .height = 10}}),
        rects.find(kThird));
}

TEST(WidgetRectsTest, Layout_KeepsTheLastDeclarationOfARepeatedId)
{
    LayoutTree tree{container(Axis::Row, kGrow, kGrow, kNoWidget)};

    tree.add(container(Axis::Row, fixedSize(10), kGrow, kFirst));
    const auto second =
        tree.add(container(Axis::Row, fixedSize(10), kGrow, kFirst));

    WidgetRects rects;

    layout(tree, Size{.width = 40, .height = 10}, &rects);

    EXPECT_EQ(1U, rects.entries.size());
    EXPECT_EQ(tree.node(second).arranged, rects.find(kFirst));
    EXPECT_EQ(10, tree.node(second).arranged.origin.x);
}

TEST(WidgetRectsTest, Frame_ReportsNothingForAUiThatNamesNothing)
{
    Context ui{Size{.width = 40, .height = 20}, plainTheme()};

    {
        const auto body = ui.column();

        ui.label("ab");
    }

    EXPECT_TRUE(ui.finish().rects.entries.empty());
}

TEST(WidgetRectsTest, Frame_ReportsTheRectangleAContainerWasFilledWith)
{
    Context ui{Size{.width = 40, .height = 20}, plainTheme()};

    {
        const auto body = ui.row({
            .height = fixedSize(8),
            .background = kAccent,
            .id = kFirst});

        ui.label("ab");
    }

    const auto frame = ui.finish();

    EXPECT_EQ(filled(frame, kAccent), frame.rects.find(kFirst));
    EXPECT_EQ(
        (Rect{
            .origin = {.x = 0, .y = 0},
            .size = {.width = 40, .height = 8}}),
        frame.rects.find(kFirst));
}

TEST(WidgetRectsTest, Frame_ReportsAButtonsRectangleToo)
{
    Context ui{Size{.width = 40, .height = 20}, plainTheme()};

    ui.button("ab", {.id = kSecond, .width = fixedSize(20)});

    const auto frame = ui.finish();

    EXPECT_EQ(
        (Rect{
            .origin = {.x = 0, .y = 0},
            .size = {.width = 20, .height = 8}}),
        frame.rects.find(kSecond));
}

TEST(WidgetRectsTest, Frame_KeepsANestedRectangleInsideItsParents)
{
    Context ui{Size{.width = 60, .height = 30}, plainTheme()};

    {
        const auto outer = ui.column({
            .width = fixedSize(40),
            .height = fixedSize(20),
            .padding = 4U,
            .id = kFirst});

        {
            const auto inner = ui.row({
                .height = fixedSize(6),
                .id = kSecond});

            ui.label("ab");
        }
    }

    const auto frame = ui.finish();
    const auto parent = frame.rects.find(kFirst);
    const auto child = frame.rects.find(kSecond);

    ASSERT_TRUE(parent.has_value());
    ASSERT_TRUE(child.has_value());

    EXPECT_GE(child->origin.x, parent->origin.x);
    EXPECT_GE(child->origin.y, parent->origin.y);
    EXPECT_LE(
        child->origin.x + static_cast<std::int32_t>(child->size.width),
        parent->origin.x
            + static_cast<std::int32_t>(parent->size.width));
    EXPECT_LE(
        child->origin.y + static_cast<std::int32_t>(child->size.height),
        parent->origin.y
            + static_cast<std::int32_t>(parent->size.height));
}

TEST(WidgetRectsTest, Frame_HoversANamedContainerLikeAnyOtherWidget)
{
    Context ui{
        Size{.width = 40, .height = 20},
        plainTheme(),
        {.position = Point{.x = 5, .y = 2}}};

    {
        const auto body = ui.row({
            .height = fixedSize(8),
            .background = kAccent,
            .id = kFirst});

        ui.label("ab");
    }

    EXPECT_EQ(kFirst, ui.finish().interactions.hovered);
}
