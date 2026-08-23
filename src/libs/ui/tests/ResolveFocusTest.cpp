#include <gtest/gtest.h>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>

#include "antwika/ui/Keyboard.hpp"
#include "antwika/ui/OptionChoice.hpp"
#include "antwika/ui/Pointer.hpp"
#include "antwika/ui/WidgetId.hpp"

#include "FocusRing.hpp"
#include "StateColors.hpp"
#include "LayoutTree.hpp"
#include "Node.hpp"
#include "Resolve.hpp"

using antwika::gfx::Color;
using antwika::gfx::Point;
using antwika::gfx::Rect;
using antwika::ui::Key;
using antwika::ui::Keyboard;
using antwika::widget::kNoWidget;
using antwika::ui::OptionChoice;
using antwika::ui::Pointer;
using antwika::widget::WidgetId;
using antwika::ui::detail::FocusRing;
using antwika::ui::detail::StateColors;
using antwika::ui::detail::LayoutTree;
using antwika::ui::detail::Node;
using antwika::ui::detail::resolve;

namespace
{
    constexpr WidgetId kFirstWidget{11};
    constexpr WidgetId kSecondWidget{22};
    constexpr WidgetId kThirdWidget{33};
    constexpr WidgetId kGoneWidget{44};

    constexpr Color kRingColor{.red = 244, .green = 208, .blue = 63};

    constexpr FocusRing kRing{.color = kRingColor, .thickness = 2};

    constexpr Rect kBoxRect{
        .originPoint = {.x = 0, .y = 0}, .size = {.width = 10, .height = 10}};

    constexpr Point kInsidePoint{.x = 5, .y = 5};

    constexpr StateColors kStyleColors{};

    Node focusable(WidgetId widget)
    {
        return Node{
            .widgetId = widget, .focusStyle = kRing, .arrangedRect = kBoxRect};
    }

    LayoutTree threeButtons()
    {
        LayoutTree tree{Node{}};

        tree.add(focusable(kFirstWidget));
        tree.add(Node{});
        tree.add(focusable(kSecondWidget));
        tree.add(focusable(kNoWidget));
        tree.add(focusable(kThirdWidget));

        return tree;
    }

    WidgetId focusAfter(
        LayoutTree &tree, Key key, WidgetId fromWidget = kNoWidget)
    {
        return resolve(tree, Pointer{}, Keyboard{.keys = {key}}, fromWidget)
            .focusedWidget;
    }
}

TEST(ResolveFocusTest, Resolve_FocusesNothingWithoutAKeyboard)
{
    auto tree = threeButtons();

    EXPECT_EQ(kNoWidget, resolve(tree, Pointer{}).focusedWidget);
}

TEST(ResolveFocusTest, Resolve_KeepsTheFocusItWasGiven)
{
    auto tree = threeButtons();

    const auto interactions =
        resolve(tree, Pointer{}, Keyboard{}, kSecondWidget);

    EXPECT_EQ(kSecondWidget, interactions.focusedWidget);
    EXPECT_EQ(kNoWidget, interactions.activatedWidget);
}

TEST(ResolveFocusTest, Resolve_TabFocusesTheFirstWidgetFromNothing)
{
    auto tree = threeButtons();

    EXPECT_EQ(kFirstWidget, focusAfter(tree, Key::FocusNext));
}

TEST(ResolveFocusTest, Resolve_ShiftTabFocusesTheLastWidgetFromNothing)
{
    auto tree = threeButtons();

    EXPECT_EQ(kThirdWidget, focusAfter(tree, Key::FocusPrevious));
}

TEST(ResolveFocusTest, Resolve_TabAdvancesInDeclarationOrder)
{
    auto tree = threeButtons();

    EXPECT_EQ(kSecondWidget, focusAfter(tree, Key::FocusNext, kFirstWidget));
    EXPECT_EQ(kThirdWidget, focusAfter(tree, Key::FocusNext, kSecondWidget));
}

TEST(ResolveFocusTest, Resolve_TabWrapsPastTheLastWidget)
{
    auto tree = threeButtons();

    EXPECT_EQ(kFirstWidget, focusAfter(tree, Key::FocusNext, kThirdWidget));
}

TEST(ResolveFocusTest, Resolve_ShiftTabGoesBack)
{
    auto tree = threeButtons();

    EXPECT_EQ(
        kSecondWidget,
        focusAfter(tree, Key::FocusPrevious, kThirdWidget));
}

TEST(ResolveFocusTest, Resolve_ShiftTabWrapsPastTheFirstWidget)
{
    auto tree = threeButtons();

    EXPECT_EQ(kThirdWidget, focusAfter(tree, Key::FocusPrevious, kFirstWidget));
}

TEST(ResolveFocusTest, Resolve_TabFocusesARootThatIsItselfFocusable)
{
    LayoutTree tree{focusable(kFirstWidget)};

    EXPECT_EQ(kFirstWidget, focusAfter(tree, Key::FocusNext));
}

TEST(ResolveFocusTest, Resolve_TabFocusesNothingWithNothingToFocus)
{
    LayoutTree tree{Node{}};
    tree.add(Node{.widgetId = kFirstWidget, .arrangedRect = kBoxRect});

    EXPECT_EQ(kNoWidget, focusAfter(tree, Key::FocusNext));
}

TEST(ResolveFocusTest, Resolve_TabStopsOnceAtARepeatedId)
{
    LayoutTree tree{Node{}};
    tree.add(focusable(kFirstWidget));
    tree.add(focusable(kFirstWidget));
    tree.add(focusable(kSecondWidget));

    EXPECT_EQ(kSecondWidget, focusAfter(tree, Key::FocusNext, kFirstWidget));
}

TEST(ResolveFocusTest, Resolve_DropsAFocusOnAWidgetThatIsGone)
{
    auto tree = threeButtons();

    EXPECT_EQ(kNoWidget, resolve(tree, Pointer{}, Keyboard{}, kGoneWidget)
                             .focusedWidget);
    EXPECT_EQ(kFirstWidget, focusAfter(tree, Key::FocusNext, kGoneWidget));
}

TEST(ResolveFocusTest, Resolve_ActivatesTheFocusedWidget)
{
    auto tree = threeButtons();

    const auto interactions = resolve(
        tree,
        Pointer{},
        Keyboard{.keys = {Key::Activate}},
        kSecondWidget);

    EXPECT_EQ(kSecondWidget, interactions.activatedWidget);
    EXPECT_EQ(kSecondWidget, interactions.focusedWidget);
}

TEST(ResolveFocusTest, Resolve_ActivatesNothingWithNothingFocused)
{
    auto tree = threeButtons();

    const auto interactions =
        resolve(tree, Pointer{}, Keyboard{.keys = {Key::Activate}});

    EXPECT_EQ(kNoWidget, interactions.activatedWidget);
}

TEST(ResolveFocusTest, Resolve_AppliesTheKeysInArrivalOrder)
{
    auto tree = threeButtons();

    const auto interactions = resolve(
        tree,
        Pointer{},
        Keyboard{.keys = {Key::FocusNext, Key::Activate}},
        kFirstWidget);

    EXPECT_EQ(kSecondWidget, interactions.activatedWidget);
    EXPECT_EQ(kSecondWidget, interactions.focusedWidget);
}

TEST(ResolveFocusTest, Resolve_MovesFocusToWhatThePointerActivated)
{
    auto tree = threeButtons();

    const auto interactions = resolve(
        tree,
        Pointer{.positionPoint = kInsidePoint, .pressed = true},
        Keyboard{},
        kFirstWidget);

    EXPECT_EQ(kThirdWidget, interactions.activatedWidget);
    EXPECT_EQ(kThirdWidget, interactions.focusedWidget);
}

TEST(ResolveFocusTest, Resolve_LeavesAPointerOnlyCallerWithoutFocus)
{
    auto tree = threeButtons();

    const auto interactions = resolve(
        tree, Pointer{.positionPoint = kInsidePoint, .pressed = true});

    EXPECT_EQ(kThirdWidget, interactions.activatedWidget);
    EXPECT_EQ(kNoWidget, interactions.focusedWidget);
}

TEST(ResolveFocusTest, Resolve_MovesFocusOnAPressOnceAKeyHasArrived)
{
    auto tree = threeButtons();

    const auto interactions = resolve(
        tree,
        Pointer{.positionPoint = kInsidePoint, .pressed = true},
        Keyboard{.keys = {Key::Activate}});

    EXPECT_EQ(kThirdWidget, interactions.focusedWidget);
}

TEST(ResolveFocusTest, Resolve_LeavesFocusAloneWhenNothingIsPressed)
{
    auto tree = threeButtons();

    const auto interactions = resolve(
        tree,
        Pointer{.positionPoint = kInsidePoint},
        Keyboard{},
        kFirstWidget);

    EXPECT_EQ(kThirdWidget, interactions.hoveredWidget);
    EXPECT_EQ(kFirstWidget, interactions.focusedWidget);
}

TEST(ResolveFocusTest, Resolve_RingsTheFocusedWidgetAndNothingElse)
{
    LayoutTree tree{Node{}};
    const auto first = tree.add(focusable(kFirstWidget));
    const auto second = tree.add(focusable(kSecondWidget));

    resolve(tree, Pointer{}, Keyboard{}, kSecondWidget);

    EXPECT_FALSE(tree.node(first).focusRing.has_value());
    ASSERT_TRUE(tree.node(second).focusRing.has_value());
    EXPECT_EQ(kRingColor, tree.node(second).focusRing->color);
}

TEST(ResolveFocusTest, Resolve_RingsNothingWithNothingFocused)
{
    LayoutTree tree{Node{}};
    const auto first = tree.add(focusable(kFirstWidget));

    resolve(tree, Pointer{});

    EXPECT_FALSE(tree.node(first).focusRing.has_value());
}

TEST(ResolveFocusTest, Resolve_RingsEveryNodeSharingTheFocusedId)
{
    LayoutTree tree{Node{}};
    const auto first = tree.add(focusable(kFirstWidget));
    const auto second = tree.add(focusable(kFirstWidget));

    resolve(tree, Pointer{}, Keyboard{}, kFirstWidget);

    EXPECT_TRUE(tree.node(first).focusRing.has_value());
    EXPECT_TRUE(tree.node(second).focusRing.has_value());
}

TEST(ResolveFocusTest, Resolve_DressesAFocusedWidgetTheHoverWayToo)
{
    LayoutTree tree{Node{}};

    Node node = focusable(kFirstWidget);
    node.styleColors = kStyleColors;

    const auto index = tree.add(std::move(node));

    resolve(
        tree, Pointer{.positionPoint = kInsidePoint}, Keyboard{}, kFirstWidget);

    EXPECT_TRUE(tree.node(index).backgroundColor.has_value());
    EXPECT_TRUE(tree.node(index).focusRing.has_value());
}

TEST(ResolveFocusTest, Resolve_ChoosesTheOptionEnterLandsOn)
{
    LayoutTree tree{Node{}};
    tree.add(focusable(kFirstWidget));

    Node optionNode = focusable(kSecondWidget);
    optionNode.optionOwnerWidget = kFirstWidget;
    optionNode.optionIndex = 3;

    tree.add(std::move(optionNode));

    const auto interactions = resolve(
        tree, Pointer{}, Keyboard{.keys = {Key::Activate}}, kSecondWidget);

    EXPECT_EQ(kSecondWidget, interactions.activatedWidget);
    ASSERT_TRUE(interactions.chosenChoice.has_value());
    EXPECT_EQ(
        (OptionChoice{.dropdownWidget = kFirstWidget, .index = 3}),
        *interactions.chosenChoice);
}

TEST(ResolveFocusTest, Resolve_ChoosesNothingWhenEnterIsOnAPlainWidget)
{
    LayoutTree tree{Node{}};
    tree.add(focusable(kFirstWidget));

    const auto interactions = resolve(
        tree, Pointer{}, Keyboard{.keys = {Key::Activate}}, kFirstWidget);

    EXPECT_EQ(kFirstWidget, interactions.activatedWidget);
    EXPECT_FALSE(interactions.chosenChoice.has_value());
}
