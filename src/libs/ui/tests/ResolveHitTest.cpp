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
using antwika::ui::kNoWidget;
using antwika::ui::Pointer;
using antwika::ui::WidgetId;
using antwika::ui::detail::LayoutTree;
using antwika::ui::detail::Node;
using antwika::ui::detail::resolve;

namespace
{
    constexpr WidgetId kFirst{1};
    constexpr WidgetId kSecond{2};

    constexpr Rect kBox{
        .origin = {.x = 10, .y = 20}, .size = {.width = 10, .height = 10}};

    Node widget(WidgetId id, Rect at)
    {
        return Node{.id = id, .arranged = at};
    }

    Pointer at(Point position)
    {
        return Pointer{.position = position};
    }

    LayoutTree oneWidget(Rect at)
    {
        LayoutTree tree{Node{}};
        tree.add(widget(kFirst, at));

        return tree;
    }
}

TEST(ResolveHitTest, Resolve_HoversTheWidgetUnderThePointer)
{
    auto tree = oneWidget(kBox);

    const auto interactions = resolve(tree, at(Point{.x = 15, .y = 25}));

    EXPECT_EQ(kFirst, interactions.hovered);
}

TEST(ResolveHitTest, Resolve_HoversNothingWithoutAPointerPosition)
{
    auto tree = oneWidget(kBox);

    const auto interactions = resolve(tree, Pointer{});

    EXPECT_EQ(kNoWidget, interactions.hovered);
}

TEST(ResolveHitTest, Resolve_CountsTheLeadingEdgeAsInside)
{
    auto tree = oneWidget(kBox);

    const auto interactions = resolve(tree, at(Point{.x = 10, .y = 20}));

    EXPECT_EQ(kFirst, interactions.hovered);
}

TEST(ResolveHitTest, Resolve_CountsTheTrailingEdgeAsOutside)
{
    auto tree = oneWidget(kBox);

    const auto interactions = resolve(tree, at(Point{.x = 20, .y = 25}));

    EXPECT_EQ(kNoWidget, interactions.hovered);
}

TEST(ResolveHitTest, Resolve_MissesAPointerLeftOfTheWidget)
{
    auto tree = oneWidget(kBox);

    const auto interactions = resolve(tree, at(Point{.x = 9, .y = 25}));

    EXPECT_EQ(kNoWidget, interactions.hovered);
}

TEST(ResolveHitTest, Resolve_MissesAPointerAboveTheWidget)
{
    auto tree = oneWidget(kBox);

    const auto interactions = resolve(tree, at(Point{.x = 15, .y = 19}));

    EXPECT_EQ(kNoWidget, interactions.hovered);
}

TEST(ResolveHitTest, Resolve_MissesAPointerBelowTheWidget)
{
    auto tree = oneWidget(kBox);

    const auto interactions = resolve(tree, at(Point{.x = 15, .y = 30}));

    EXPECT_EQ(kNoWidget, interactions.hovered);
}

TEST(ResolveHitTest, Resolve_MissesACollapsedWidget)
{
    auto tree = oneWidget(
        Rect{.origin = {.x = 10, .y = 20}, .size = {.width = 0, .height = 0}});

    const auto interactions = resolve(tree, at(Point{.x = 10, .y = 20}));

    EXPECT_EQ(kNoWidget, interactions.hovered);
}

TEST(ResolveHitTest, Resolve_PrefersTheWidgetDeclaredLast)
{
    LayoutTree tree{Node{}};
    tree.add(widget(kFirst, kBox));
    tree.add(widget(kSecond, kBox));

    const auto interactions = resolve(tree, at(Point{.x = 15, .y = 25}));

    EXPECT_EQ(kSecond, interactions.hovered);
}

TEST(ResolveHitTest, Resolve_IgnoresUnnamedChildrenOfANamedWidget)
{
    LayoutTree tree{Node{}};
    const auto button = tree.open(widget(kFirst, kBox));
    tree.add(Node{.arranged = kBox});
    tree.close();

    const auto interactions = resolve(tree, at(Point{.x = 15, .y = 25}));

    EXPECT_EQ(kFirst, interactions.hovered);
    EXPECT_EQ(kFirst, tree.node(button).id);
}
