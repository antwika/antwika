#include <gtest/gtest.h>

#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>

#include "antwika/ui/Interactions.hpp"
#include "antwika/ui/Pointer.hpp"
#include "antwika/ui/WidgetId.hpp"

#include "LayoutTree.hpp"
#include "Node.hpp"
#include "Resolve.hpp"

using antwika::gfx::Point;
using antwika::gfx::Rect;
using antwika::widget::kNoWidget;
using antwika::ui::Pointer;
using antwika::widget::WidgetId;
using antwika::ui::detail::LayoutTree;
using antwika::ui::detail::Node;
using antwika::ui::detail::resolve;

namespace
{
    constexpr WidgetId kFirstWidget{1};
    constexpr WidgetId kSecondWidget{2};

    constexpr Rect kBoxRect{
        .originPoint = {.x = 10, .y = 20}, .size = {.width = 10, .height = 10}};

    Node getWidget(WidgetId widgetId, Rect rect)
    {
        return Node{.widgetId = widgetId, .arrangedRect = rect};
    }

    Pointer getEntryAt(Point positionPoint)
    {
        return Pointer{.positionPoint = positionPoint};
    }

    LayoutTree getOneWidget(Rect rect)
    {
        LayoutTree tree{Node{}};
        tree.add(getWidget(kFirstWidget, rect));

        return tree;
    }
}

TEST(ResolveHitTest, Resolve_HoversTheWidgetUnderThePointer)
{
    auto tree = getOneWidget(kBoxRect);

    const auto interactions = resolve(tree, getEntryAt(Point{.x = 15, .y = 25}));

    EXPECT_EQ(kFirstWidget, interactions.hoveredWidget);
}

TEST(ResolveHitTest, Resolve_HoversNothingWithoutAPointerPosition)
{
    auto tree = getOneWidget(kBoxRect);

    const auto interactions = resolve(tree, Pointer{});

    EXPECT_EQ(kNoWidget, interactions.hoveredWidget);
}

TEST(ResolveHitTest, Resolve_CountsTheLeadingEdgeAsInside)
{
    auto tree = getOneWidget(kBoxRect);

    const auto interactions = resolve(tree, getEntryAt(Point{.x = 10, .y = 20}));

    EXPECT_EQ(kFirstWidget, interactions.hoveredWidget);
}

TEST(ResolveHitTest, Resolve_CountsTheTrailingEdgeAsOutside)
{
    auto tree = getOneWidget(kBoxRect);

    const auto interactions = resolve(tree, getEntryAt(Point{.x = 20, .y = 25}));

    EXPECT_EQ(kNoWidget, interactions.hoveredWidget);
}

TEST(ResolveHitTest, Resolve_MissesAPointerLeftOfTheWidget)
{
    auto tree = getOneWidget(kBoxRect);

    const auto interactions = resolve(tree, getEntryAt(Point{.x = 9, .y = 25}));

    EXPECT_EQ(kNoWidget, interactions.hoveredWidget);
}

TEST(ResolveHitTest, Resolve_MissesAPointerAboveTheWidget)
{
    auto tree = getOneWidget(kBoxRect);

    const auto interactions = resolve(tree, getEntryAt(Point{.x = 15, .y = 19}));

    EXPECT_EQ(kNoWidget, interactions.hoveredWidget);
}

TEST(ResolveHitTest, Resolve_MissesAPointerBelowTheWidget)
{
    auto tree = getOneWidget(kBoxRect);

    const auto interactions = resolve(tree, getEntryAt(Point{.x = 15, .y = 30}));

    EXPECT_EQ(kNoWidget, interactions.hoveredWidget);
}

TEST(ResolveHitTest, Resolve_MissesACollapsedWidget)
{
    auto tree = getOneWidget(
        Rect{
            .originPoint = {.x = 10, .y = 20},
            .size = {.width = 0, .height = 0}});

    const auto interactions = resolve(tree, getEntryAt(Point{.x = 10, .y = 20}));

    EXPECT_EQ(kNoWidget, interactions.hoveredWidget);
}

TEST(ResolveHitTest, Resolve_PrefersTheWidgetDeclaredLast)
{
    LayoutTree tree{Node{}};
    tree.add(getWidget(kFirstWidget, kBoxRect));
    tree.add(getWidget(kSecondWidget, kBoxRect));

    const auto interactions = resolve(tree, getEntryAt(Point{.x = 15, .y = 25}));

    EXPECT_EQ(kSecondWidget, interactions.hoveredWidget);
}

TEST(ResolveHitTest, Resolve_IgnoresUnnamedChildrenOfANamedWidget)
{
    LayoutTree tree{Node{}};
    const auto button = tree.open(getWidget(kFirstWidget, kBoxRect));
    tree.add(Node{.arrangedRect = kBoxRect});
    tree.close();

    const auto interactions = resolve(tree, getEntryAt(Point{.x = 15, .y = 25}));

    EXPECT_EQ(kFirstWidget, interactions.hoveredWidget);
    EXPECT_EQ(kFirstWidget, tree.getNode(button).widgetId);
}
