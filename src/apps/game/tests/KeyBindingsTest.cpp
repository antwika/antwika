#include <gtest/gtest.h>

#include <set>

#include <antwika/input/Key.hpp>

#include "antwika/game/Action.hpp"
#include "antwika/game/KeyBindings.hpp"

using antwika::game::Action;
using antwika::game::BindOutcome;
using antwika::game::isReservedKey;
using antwika::game::kActionCount;
using antwika::game::kActions;
using antwika::game::kDefaultBindings;
using antwika::game::kFullscreenKey;
using antwika::game::KeyBindings;
using antwika::game::kQuitKey;
using antwika::game::kReservedKeys;
using antwika::input::Key;

// The invariant every other answer here rests on.
// Asserted rather than claimed in a comment beside the table.
TEST(KeyBindingsTest, TheShippedLayoutBindsEveryActionToAKeyOfItsOwn)
{
    std::set<Key> keys;

    for (const auto action : kActions)
    {
        keys.insert(kDefaultBindings.keyFor(action));
        EXPECT_FALSE(isReservedKey(kDefaultBindings.keyFor(action)));
    }

    EXPECT_EQ(keys.size(), kActionCount);
}

TEST(KeyBindingsTest, TheShippedLayoutAnswersForEveryKeyItHolds)
{
    for (const auto action : kActions)
    {
        EXPECT_EQ(
            kDefaultBindings.actionFor(kDefaultBindings.keyFor(action)),
            action);
    }
}

TEST(KeyBindingsTest, AKeyNothingHoldsAsksForNothing)
{
    EXPECT_FALSE(kDefaultBindings.actionFor(Key::J).has_value());
}

TEST(KeyBindingsTest, BindingAFreeKeyMovesTheAction)
{
    KeyBindings bindings;

    EXPECT_EQ(bindings.bind(Action::Pause, Key::J), BindOutcome::Bound);
    EXPECT_EQ(bindings.keyFor(Action::Pause), Key::J);
    EXPECT_EQ(bindings.actionFor(Key::J), Action::Pause);

    // And the key it left is free again.
    EXPECT_FALSE(
        bindings.actionFor(kDefaultBindings.keyFor(Action::Pause))
            .has_value());
}

// Asked before anything else.
// So rebinding an action to the key it already holds.
// Is not reported as somebody else holding it.
TEST(KeyBindingsTest, BindingTheKeyAnActionAlreadyHoldsChangesNothing)
{
    KeyBindings bindings;

    EXPECT_EQ(
        bindings.bind(Action::Pause, kDefaultBindings.keyFor(Action::Pause)),
        BindOutcome::Unchanged);
    EXPECT_EQ(bindings, kDefaultBindings);
}

TEST(KeyBindingsTest, AKeyAnotherActionHoldsIsRefused)
{
    KeyBindings bindings;

    EXPECT_EQ(
        bindings.bind(
            Action::Pause, kDefaultBindings.keyFor(Action::ZoomIn)),
        BindOutcome::Taken);
    EXPECT_EQ(bindings, kDefaultBindings);
}

// The two this application spends above the tick loop.
// A binding on either would fire and quit or fill the screen as well.
TEST(KeyBindingsTest, AKeyTheApplicationSpendsElsewhereIsRefused)
{
    KeyBindings bindings;

    for (const auto key : kReservedKeys)
    {
        EXPECT_TRUE(isReservedKey(key));
        EXPECT_EQ(
            bindings.bind(Action::Pause, key), BindOutcome::Reserved);
    }

    EXPECT_EQ(bindings, kDefaultBindings);
}

TEST(KeyBindingsTest, OnlyTheReservedKeysAreReserved)
{
    EXPECT_TRUE(isReservedKey(kQuitKey));
    EXPECT_TRUE(isReservedKey(kFullscreenKey));
    EXPECT_FALSE(isReservedKey(Key::J));
    EXPECT_FALSE(isReservedKey(Key::F9));
}

// Two layouts that bind the same things are the same layout.
// Which is what lets a summary comparison catch a divergence.
TEST(KeyBindingsTest, TwoLayoutsCompareByWhatTheyBind)
{
    KeyBindings one;
    KeyBindings other;

    EXPECT_EQ(one, other);

    EXPECT_EQ(one.bind(Action::ResetView, Key::J), BindOutcome::Bound);
    EXPECT_NE(one, other);

    EXPECT_EQ(other.bind(Action::ResetView, Key::J), BindOutcome::Bound);
    EXPECT_EQ(one, other);
}
