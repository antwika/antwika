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

TEST(KeyBindingsTest, KeyFor_BindsEveryActionToItsOwnKey)
{
    std::set<Key> keys;

    for (const auto action : kActions)
    {
        keys.insert(kDefaultBindings.keyFor(action));
        EXPECT_FALSE(isReservedKey(kDefaultBindings.keyFor(action)));
    }

    EXPECT_EQ(keys.size(), kActionCount);
}

TEST(KeyBindingsTest, ActionFor_AnswersForEveryBoundKey)
{
    for (const auto action : kActions)
    {
        EXPECT_EQ(
            kDefaultBindings.actionFor(kDefaultBindings.keyFor(action)),
            action);
    }
}

TEST(KeyBindingsTest, ActionFor_AnswersNothingForAFreeKey)
{
    EXPECT_FALSE(kDefaultBindings.actionFor(Key::J).has_value());
}

TEST(KeyBindingsTest, Bind_BindingAFreeKeyMovesTheAction)
{
    KeyBindings bindings;

    EXPECT_EQ(bindings.bind(Action::Pause, Key::J), BindOutcome::Bound);
    EXPECT_EQ(bindings.keyFor(Action::Pause), Key::J);
    EXPECT_EQ(bindings.actionFor(Key::J), Action::Pause);

    EXPECT_FALSE(
        bindings.actionFor(kDefaultBindings.keyFor(Action::Pause))
            .has_value());
}

TEST(KeyBindingsTest, Bind_ChangesNothingOnItsOwnKey)
{
    KeyBindings bindings;

    EXPECT_EQ(
        bindings.bind(Action::Pause, kDefaultBindings.keyFor(Action::Pause)),
        BindOutcome::Unchanged);
    EXPECT_EQ(bindings, kDefaultBindings);
}

TEST(KeyBindingsTest, Bind_RefusesAnotherActionsKey)
{
    KeyBindings bindings;

    EXPECT_EQ(
        bindings.bind(
            Action::Pause, kDefaultBindings.keyFor(Action::ZoomIn)),
        BindOutcome::Taken);
    EXPECT_EQ(bindings, kDefaultBindings);
}

TEST(KeyBindingsTest, Bind_RefusesAReservedKey)
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

TEST(KeyBindingsTest, IsReservedKey_OnlyTheReservedKeysAreReserved)
{
    EXPECT_TRUE(isReservedKey(kQuitKey));
    EXPECT_TRUE(isReservedKey(kFullscreenKey));
    EXPECT_FALSE(isReservedKey(Key::J));
    EXPECT_FALSE(isReservedKey(Key::F9));
}

TEST(KeyBindingsTest, OperatorEquals_ComparesWhatTheyBind)
{
    KeyBindings one;
    KeyBindings other;

    EXPECT_EQ(one, other);

    EXPECT_EQ(one.bind(Action::ResetView, Key::J), BindOutcome::Bound);
    EXPECT_NE(one, other);

    EXPECT_EQ(other.bind(Action::ResetView, Key::J), BindOutcome::Bound);
    EXPECT_EQ(one, other);
}
