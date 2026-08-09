#include <gtest/gtest.h>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>

#include "antwika/ui/Keyboard.hpp"
#include "antwika/ui/OptionChoice.hpp"
#include "antwika/ui/Pointer.hpp"
#include "antwika/ui/WidgetId.hpp"

#include "FocusRing.hpp"
#include "Interactive.hpp"
#include "LayoutTree.hpp"
#include "Node.hpp"
#include "Resolve.hpp"

using antwika::gfx::Color;
using antwika::gfx::Point;
using antwika::gfx::Rect;
using antwika::ui::Key;
using antwika::ui::Keyboard;
using antwika::ui::kNoWidget;
using antwika::ui::OptionChoice;
using antwika::ui::Pointer;
using antwika::ui::WidgetId;
using antwika::ui::detail::FocusRing;
using antwika::ui::detail::Interactive;
using antwika::ui::detail::LayoutTree;
using antwika::ui::detail::Node;
using antwika::ui::detail::resolve;

namespace
{
    constexpr WidgetId kFirst{11};
    constexpr WidgetId kSecond{22};
    constexpr WidgetId kThird{33};
    constexpr WidgetId kGone{44};

    constexpr Color kRingColor{.red = 244, .green = 208, .blue = 63};

    constexpr FocusRing kRing{.color = kRingColor, .thickness = 2};

    constexpr Rect kBox{
        .origin = {.x = 0, .y = 0}, .size = {.width = 10, .height = 10}};

    constexpr Point kInside{.x = 5, .y = 5};

    constexpr Interactive kStyle{};

    Node focusable(WidgetId id)
    {
        return Node{.id = id, .focusStyle = kRing, .arranged = kBox};
    }

    LayoutTree threeButtons()
    {
        LayoutTree tree{Node{}};

        tree.add(focusable(kFirst));
        tree.add(Node{});
        tree.add(focusable(kSecond));
        tree.add(focusable(kNoWidget));
        tree.add(focusable(kThird));

        return tree;
    }

    WidgetId focusAfter(
        LayoutTree &tree, Key key, WidgetId from = kNoWidget)
    {
        return resolve(tree, Pointer{}, Keyboard{.keys = {key}}, from)
            .focused;
    }
}

TEST(ResolveFocusTest, Resolve_FocusesNothingWithoutAKeyboard)
{
    auto tree = threeButtons();

    EXPECT_EQ(kNoWidget, resolve(tree, Pointer{}).focused);
}

TEST(ResolveFocusTest, Resolve_KeepsTheFocusItWasGiven)
{
    auto tree = threeButtons();

    const auto interactions =
        resolve(tree, Pointer{}, Keyboard{}, kSecond);

    EXPECT_EQ(kSecond, interactions.focused);
    EXPECT_EQ(kNoWidget, interactions.activated);
}

TEST(ResolveFocusTest, Resolve_TabFocusesTheFirstWidgetFromNothing)
{
    auto tree = threeButtons();

    EXPECT_EQ(kFirst, focusAfter(tree, Key::FocusNext));
}

TEST(ResolveFocusTest, Resolve_ShiftTabFocusesTheLastWidgetFromNothing)
{
    auto tree = threeButtons();

    EXPECT_EQ(kThird, focusAfter(tree, Key::FocusPrevious));
}

TEST(ResolveFocusTest, Resolve_TabAdvancesInDeclarationOrder)
{
    auto tree = threeButtons();

    EXPECT_EQ(kSecond, focusAfter(tree, Key::FocusNext, kFirst));
    EXPECT_EQ(kThird, focusAfter(tree, Key::FocusNext, kSecond));
}

TEST(ResolveFocusTest, Resolve_TabWrapsPastTheLastWidget)
{
    auto tree = threeButtons();

    EXPECT_EQ(kFirst, focusAfter(tree, Key::FocusNext, kThird));
}

TEST(ResolveFocusTest, Resolve_ShiftTabGoesBack)
{
    auto tree = threeButtons();

    EXPECT_EQ(kSecond, focusAfter(tree, Key::FocusPrevious, kThird));
}

TEST(ResolveFocusTest, Resolve_ShiftTabWrapsPastTheFirstWidget)
{
    auto tree = threeButtons();

    EXPECT_EQ(kThird, focusAfter(tree, Key::FocusPrevious, kFirst));
}

TEST(ResolveFocusTest, Resolve_TabFocusesARootThatIsItselfFocusable)
{
    LayoutTree tree{focusable(kFirst)};

    EXPECT_EQ(kFirst, focusAfter(tree, Key::FocusNext));
}

TEST(ResolveFocusTest, Resolve_TabFocusesNothingWithNothingToFocus)
{
    LayoutTree tree{Node{}};
    tree.add(Node{.id = kFirst, .arranged = kBox});

    EXPECT_EQ(kNoWidget, focusAfter(tree, Key::FocusNext));
}

TEST(ResolveFocusTest, Resolve_TabStopsOnceAtARepeatedId)
{
    LayoutTree tree{Node{}};
    tree.add(focusable(kFirst));
    tree.add(focusable(kFirst));
    tree.add(focusable(kSecond));

    EXPECT_EQ(kSecond, focusAfter(tree, Key::FocusNext, kFirst));
}

TEST(ResolveFocusTest, Resolve_DropsAFocusOnAWidgetThatIsGone)
{
    auto tree = threeButtons();

    EXPECT_EQ(kNoWidget, resolve(tree, Pointer{}, Keyboard{}, kGone)
                             .focused);
    EXPECT_EQ(kFirst, focusAfter(tree, Key::FocusNext, kGone));
}

TEST(ResolveFocusTest, Resolve_ActivatesTheFocusedWidget)
{
    auto tree = threeButtons();

    const auto interactions = resolve(
        tree,
        Pointer{},
        Keyboard{.keys = {Key::Activate}},
        kSecond);

    EXPECT_EQ(kSecond, interactions.activated);
    EXPECT_EQ(kSecond, interactions.focused);
}

TEST(ResolveFocusTest, Resolve_ActivatesNothingWithNothingFocused)
{
    auto tree = threeButtons();

    const auto interactions =
        resolve(tree, Pointer{}, Keyboard{.keys = {Key::Activate}});

    EXPECT_EQ(kNoWidget, interactions.activated);
}

TEST(ResolveFocusTest, Resolve_AppliesTheKeysInArrivalOrder)
{
    auto tree = threeButtons();

    const auto interactions = resolve(
        tree,
        Pointer{},
        Keyboard{.keys = {Key::FocusNext, Key::Activate}},
        kFirst);

    EXPECT_EQ(kSecond, interactions.activated);
    EXPECT_EQ(kSecond, interactions.focused);
}

TEST(ResolveFocusTest, Resolve_MovesFocusToWhatThePointerActivated)
{
    auto tree = threeButtons();

    const auto interactions = resolve(
        tree,
        Pointer{.position = kInside, .pressed = true},
        Keyboard{},
        kFirst);

    EXPECT_EQ(kThird, interactions.activated);
    EXPECT_EQ(kThird, interactions.focused);
}

TEST(ResolveFocusTest, Resolve_LeavesAPointerOnlyCallerWithoutFocus)
{
    auto tree = threeButtons();

    const auto interactions = resolve(
        tree, Pointer{.position = kInside, .pressed = true});

    EXPECT_EQ(kThird, interactions.activated);
    EXPECT_EQ(kNoWidget, interactions.focused);
}

TEST(ResolveFocusTest, Resolve_MovesFocusOnAPressOnceAKeyHasArrived)
{
    auto tree = threeButtons();

    const auto interactions = resolve(
        tree,
        Pointer{.position = kInside, .pressed = true},
        Keyboard{.keys = {Key::Activate}});

    EXPECT_EQ(kThird, interactions.focused);
}

TEST(ResolveFocusTest, Resolve_LeavesFocusAloneWhenNothingIsPressed)
{
    auto tree = threeButtons();

    const auto interactions = resolve(
        tree,
        Pointer{.position = kInside},
        Keyboard{},
        kFirst);

    EXPECT_EQ(kThird, interactions.hovered);
    EXPECT_EQ(kFirst, interactions.focused);
}

TEST(ResolveFocusTest, Resolve_RingsTheFocusedWidgetAndNothingElse)
{
    LayoutTree tree{Node{}};
    const auto first = tree.add(focusable(kFirst));
    const auto second = tree.add(focusable(kSecond));

    resolve(tree, Pointer{}, Keyboard{}, kSecond);

    EXPECT_FALSE(tree.node(first).focusRing.has_value());
    ASSERT_TRUE(tree.node(second).focusRing.has_value());
    EXPECT_EQ(kRingColor, tree.node(second).focusRing->color);
}

TEST(ResolveFocusTest, Resolve_RingsNothingWithNothingFocused)
{
    LayoutTree tree{Node{}};
    const auto first = tree.add(focusable(kFirst));

    resolve(tree, Pointer{});

    EXPECT_FALSE(tree.node(first).focusRing.has_value());
}

TEST(ResolveFocusTest, Resolve_RingsEveryNodeSharingTheFocusedId)
{
    LayoutTree tree{Node{}};
    const auto first = tree.add(focusable(kFirst));
    const auto second = tree.add(focusable(kFirst));

    resolve(tree, Pointer{}, Keyboard{}, kFirst);

    EXPECT_TRUE(tree.node(first).focusRing.has_value());
    EXPECT_TRUE(tree.node(second).focusRing.has_value());
}

TEST(ResolveFocusTest, Resolve_DressesAFocusedWidgetTheHoverWayToo)
{
    LayoutTree tree{Node{}};

    Node node = focusable(kFirst);
    node.style = kStyle;

    const auto index = tree.add(std::move(node));

    resolve(
        tree, Pointer{.position = kInside}, Keyboard{}, kFirst);

    EXPECT_TRUE(tree.node(index).background.has_value());
    EXPECT_TRUE(tree.node(index).focusRing.has_value());
}

TEST(ResolveFocusTest, Resolve_ChoosesTheOptionEnterLandsOn)
{
    LayoutTree tree{Node{}};
    tree.add(focusable(kFirst));

    Node option = focusable(kSecond);
    option.optionOwner = kFirst;
    option.optionIndex = 3;

    tree.add(std::move(option));

    const auto interactions = resolve(
        tree, Pointer{}, Keyboard{.keys = {Key::Activate}}, kSecond);

    EXPECT_EQ(kSecond, interactions.activated);
    ASSERT_TRUE(interactions.chosen.has_value());
    EXPECT_EQ(
        (OptionChoice{.dropdown = kFirst, .index = 3}),
        *interactions.chosen);
}

TEST(ResolveFocusTest, Resolve_ChoosesNothingWhenEnterIsOnAPlainWidget)
{
    LayoutTree tree{Node{}};
    tree.add(focusable(kFirst));

    const auto interactions = resolve(
        tree, Pointer{}, Keyboard{.keys = {Key::Activate}}, kFirst);

    EXPECT_EQ(kFirst, interactions.activated);
    EXPECT_FALSE(interactions.chosen.has_value());
}
