#include <gtest/gtest.h>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>

#include "antwika/ui/Interactions.hpp"
#include "antwika/ui/Pointer.hpp"
#include "antwika/ui/WidgetId.hpp"

#include "StateColors.hpp"
#include "LayoutTree.hpp"
#include "Node.hpp"
#include "Resolve.hpp"

using antwika::gfx::Color;
using antwika::gfx::Point;
using antwika::gfx::Rect;
using antwika::widget::kNoWidget;
using antwika::ui::Pointer;
using antwika::widget::WidgetId;
using antwika::ui::detail::StateColors;
using antwika::ui::detail::LayoutTree;
using antwika::ui::detail::Node;
using antwika::ui::detail::resolve;

namespace
{
    constexpr WidgetId kButtonWidget{7};

    constexpr Rect kBoxRect{
        .originPoint = {.x = 0, .y = 0}, .size = {.width = 10, .height = 10}};

    constexpr Color kIdleColor{.red = 1};
    constexpr Color kHoveredColor{.red = 2};
    constexpr Color kPressedColor{.red = 3};

    constexpr StateColors kStyleColors{
        .idleColor = kIdleColor,
        .hoveredColor = kHoveredColor,
        .pressedColor = kPressedColor};

    constexpr Point kInsidePoint{.x = 5, .y = 5};
    constexpr Point kOutsidePoint{.x = 50, .y = 50};

    Node getButton(WidgetId widget)
    {
        return Node{
            .backgroundColor = kIdleColor,
            .widgetId = widget,
            .styleColors = kStyleColors,
            .arrangedRect = kBoxRect};
    }
}

TEST(ResolveTest, Resolve_ActivatesTheWidgetAPressLandsOn)
{
    LayoutTree tree{Node{}};
    tree.add(getButton(kButtonWidget));

    const auto interactions =
        resolve(tree, Pointer{.positionPoint = kInsidePoint, .pressed = true});

    EXPECT_EQ(kButtonWidget, interactions.activatedWidget);
}

TEST(ResolveTest, Resolve_ActivatesNothingWhenAPressLandsOnNothing)
{
    LayoutTree tree{Node{}};
    tree.add(getButton(kButtonWidget));

    const auto interactions =
        resolve(tree, Pointer{.positionPoint = kOutsidePoint, .pressed = true});

    EXPECT_EQ(kNoWidget, interactions.activatedWidget);
}

TEST(ResolveTest, Resolve_ActivatesNothingWithoutAPress)
{
    LayoutTree tree{Node{}};
    tree.add(
        getButton(kButtonWidget));

    const auto interactions = resolve(
        tree,
        Pointer{.positionPoint = kInsidePoint});

    EXPECT_EQ(kButtonWidget, interactions.hoveredWidget);
    EXPECT_EQ(kNoWidget, interactions.activatedWidget);
}

TEST(ResolveTest, Resolve_ReportsThePointerOverAFilledNode)
{
    LayoutTree tree{Node{}};
    tree.add(
        Node{.backgroundColor = kIdleColor, .arrangedRect = kBoxRect});

    const auto interactions = resolve(
        tree,
        Pointer{.positionPoint = kInsidePoint});

    EXPECT_TRUE(interactions.pointerOverUi);
    EXPECT_EQ(kNoWidget, interactions.hoveredWidget);
}

TEST(ResolveTest, Resolve_ReportsThePointerNotOverAnUnfilledNode)
{
    LayoutTree tree{Node{}};
    tree.add(
        Node{.widgetId = kButtonWidget, .arrangedRect = kBoxRect});

    const auto interactions = resolve(
        tree,
        Pointer{.positionPoint = kInsidePoint});

    EXPECT_FALSE(interactions.pointerOverUi);
    EXPECT_EQ(kButtonWidget, interactions.hoveredWidget);
}

TEST(ResolveTest, Resolve_DressesAWidgetThePointerIsOffAsIdle)
{
    LayoutTree tree{Node{}};
    const auto node = tree.add(getButton(kButtonWidget));

    resolve(tree, Pointer{.positionPoint = kOutsidePoint});

    EXPECT_EQ(kIdleColor, tree.getNode(node).backgroundColor);
}

TEST(ResolveTest, Resolve_DressesAWidgetUnderThePointerAsHovered)
{
    LayoutTree tree{Node{}};
    const auto node = tree.add(getButton(kButtonWidget));

    resolve(tree, Pointer{.positionPoint = kInsidePoint});

    EXPECT_EQ(kHoveredColor, tree.getNode(node).backgroundColor);
}

TEST(ResolveTest, Resolve_DressesAHeldWidgetAsPressed)
{
    LayoutTree tree{Node{}};
    const auto node = tree.add(getButton(kButtonWidget));

    resolve(tree, Pointer{.positionPoint = kInsidePoint, .down = true});

    EXPECT_EQ(kPressedColor, tree.getNode(node).backgroundColor);
}

TEST(ResolveTest, Resolve_LeavesAnUnnamedWidgetIdle)
{
    LayoutTree tree{Node{}};
    const auto node = tree.add(getButton(kNoWidget));

    resolve(tree, Pointer{.positionPoint = kInsidePoint, .down = true});

    EXPECT_EQ(kIdleColor, tree.getNode(node).backgroundColor);
}

TEST(ResolveTest, Resolve_LeavesANodeWithNoStyleAlone)
{
    LayoutTree tree{Node{}};
    const auto node =
        tree.add(Node{
            .backgroundColor = kPressedColor,
            .widgetId = kButtonWidget,
            .arrangedRect = kBoxRect});

    resolve(tree, Pointer{.positionPoint = kInsidePoint});

    EXPECT_EQ(kPressedColor, tree.getNode(node).backgroundColor);
}
