#include <gtest/gtest.h>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>

#include "antwika/ui/Interactions.hpp"
#include "antwika/ui/Pointer.hpp"
#include "antwika/ui/WidgetId.hpp"

#include "Interactive.hpp"
#include "LayoutTree.hpp"
#include "Node.hpp"
#include "Resolve.hpp"

using antwika::gfx::Color;
using antwika::gfx::Point;
using antwika::gfx::Rect;
using antwika::ui::kNoWidget;
using antwika::ui::Pointer;
using antwika::ui::WidgetId;
using antwika::ui::detail::Interactive;
using antwika::ui::detail::LayoutTree;
using antwika::ui::detail::Node;
using antwika::ui::detail::resolve;

namespace
{
    constexpr WidgetId kButton{7};

    constexpr Rect kBox{
        .origin = {.x = 0, .y = 0}, .size = {.width = 10, .height = 10}};

    constexpr Color kIdle{.red = 1};
    constexpr Color kHovered{.red = 2};
    constexpr Color kPressed{.red = 3};

    constexpr Interactive kStyle{
        .idle = kIdle, .hovered = kHovered, .pressed = kPressed};

    constexpr Point kInside{.x = 5, .y = 5};
    constexpr Point kOutside{.x = 50, .y = 50};

    Node button(WidgetId id)
    {
        return Node{
            .background = kIdle, .id = id, .style = kStyle, .arranged = kBox};
    }
} // namespace

TEST(ResolveTest, Resolve_ActivatesTheWidgetAPressLandsOn)
{
    LayoutTree tree{Node{}};
    tree.add(button(kButton));

    const auto interactions =
        resolve(tree, Pointer{.position = kInside, .pressed = true});

    EXPECT_EQ(kButton, interactions.activated);
}

TEST(ResolveTest, Resolve_ActivatesNothingWhenAPressLandsOnNothing)
{
    LayoutTree tree{Node{}};
    tree.add(button(kButton));

    const auto interactions =
        resolve(tree, Pointer{.position = kOutside, .pressed = true});

    EXPECT_EQ(kNoWidget, interactions.activated);
}

TEST(ResolveTest, Resolve_ActivatesNothingWithoutAPress)
{
    LayoutTree tree{Node{}};
    tree.add(button(kButton));

    const auto interactions = resolve(tree, Pointer{.position = kInside});

    EXPECT_EQ(kButton, interactions.hovered);
    EXPECT_EQ(kNoWidget, interactions.activated);
}

TEST(ResolveTest, Resolve_ReportsThePointerOverAFilledNode)
{
    LayoutTree tree{Node{}};
    tree.add(Node{.background = kIdle, .arranged = kBox});

    const auto interactions = resolve(tree, Pointer{.position = kInside});

    // Unnamed, but it covers what was drawn underneath it.
    EXPECT_TRUE(interactions.pointerOverUi);
    EXPECT_EQ(kNoWidget, interactions.hovered);
}

TEST(ResolveTest, Resolve_ReportsThePointerNotOverAnUnfilledNode)
{
    LayoutTree tree{Node{}};
    tree.add(Node{.id = kButton, .arranged = kBox});

    const auto interactions = resolve(tree, Pointer{.position = kInside});

    EXPECT_FALSE(interactions.pointerOverUi);
    EXPECT_EQ(kButton, interactions.hovered);
}

TEST(ResolveTest, Resolve_DressesAWidgetThePointerIsOffAsIdle)
{
    LayoutTree tree{Node{}};
    const auto node = tree.add(button(kButton));

    resolve(tree, Pointer{.position = kOutside});

    EXPECT_EQ(kIdle, tree.node(node).background);
}

TEST(ResolveTest, Resolve_DressesAWidgetUnderThePointerAsHovered)
{
    LayoutTree tree{Node{}};
    const auto node = tree.add(button(kButton));

    resolve(tree, Pointer{.position = kInside});

    EXPECT_EQ(kHovered, tree.node(node).background);
}

TEST(ResolveTest, Resolve_DressesAHeldWidgetAsPressed)
{
    LayoutTree tree{Node{}};
    const auto node = tree.add(button(kButton));

    resolve(tree, Pointer{.position = kInside, .down = true});

    EXPECT_EQ(kPressed, tree.node(node).background);
}

TEST(ResolveTest, Resolve_LeavesAnUnnamedWidgetIdle)
{
    LayoutTree tree{Node{}};
    const auto node = tree.add(button(kNoWidget));

    // Nothing can hover what the caller did not name, so two kNoWidgets
    // meeting must not read as a match.
    resolve(tree, Pointer{.position = kInside, .down = true});

    EXPECT_EQ(kIdle, tree.node(node).background);
}

TEST(ResolveTest, Resolve_LeavesANodeWithNoStyleAlone)
{
    LayoutTree tree{Node{}};
    const auto node =
        tree.add(Node{.background = kPressed, .id = kButton, .arranged = kBox});

    resolve(tree, Pointer{.position = kInside});

    // What a button whose caller said how it must look relies on.
    EXPECT_EQ(kPressed, tree.node(node).background);
}
