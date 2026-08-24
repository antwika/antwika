#include <gtest/gtest.h>

#include <antwika/editor/editor/state/KeyBench.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/input/KeyModifiers.hpp>

using antwika::editor::Action;
using antwika::editor::Chord;
using antwika::editor::KeyBench;
using antwika::editor::KeyBindings;
using antwika::input::Key;
using antwika::input::KeyModifiers;

TEST(KeyBenchTest, TakeBindings_MatchesTheChordItWasHandedAfterwards)
{
    KeyBench bench;
    KeyBindings keyBindings;

    keyBindings[Action::Save] = Chord{.key = Key::S, .ctrl = true};
    bench.takeBindings(keyBindings);

    EXPECT_TRUE(
        bench.matchesChord(
            Action::Save, Key::S, KeyModifiers{.control = true}));
}

TEST(KeyBenchTest, TakeBindings_LeavesTheChordItReplaced)
{
    KeyBench bench;
    KeyBindings keyBindings;

    keyBindings[Action::Save] = Chord{.key = Key::S, .ctrl = true};
    bench.takeBindings(keyBindings);

    keyBindings[Action::Save] = Chord{.key = Key::W, .ctrl = true};
    bench.takeBindings(keyBindings);

    EXPECT_FALSE(
        bench.matchesChord(
            Action::Save, Key::S, KeyModifiers{.control = true}));
    EXPECT_TRUE(
        bench.matchesChord(
            Action::Save, Key::W, KeyModifiers{.control = true}));
}

TEST(KeyBenchTest, MatchesChord_RefusesAChordMissingItsModifiers)
{
    KeyBench bench;
    KeyBindings keyBindings;

    keyBindings[Action::Save] = Chord{.key = Key::S, .ctrl = true};
    bench.takeBindings(keyBindings);

    EXPECT_FALSE(bench.matchesChord(Action::Save, Key::S, KeyModifiers{}));
}

TEST(KeyBenchTest, Bindings_HandBackWhatWasTakenIn)
{
    KeyBench bench;
    KeyBindings keyBindings;

    keyBindings[Action::Save] = Chord{.key = Key::S, .ctrl = true};
    bench.takeBindings(keyBindings);

    ASSERT_TRUE(bench.getBindings().at(Action::Save).has_value());
    EXPECT_EQ(bench.getBindings().at(Action::Save)->key, Key::S);
}

TEST(KeyBenchTest, KeyBench_RestsWithNothingRebindingAndThePanelShut)
{
    const KeyBench bench;

    EXPECT_FALSE(bench.panelShown);
    EXPECT_FALSE(bench.rebindingAction.has_value());
    EXPECT_TRUE(bench.typedThisFrame.empty());
    EXPECT_TRUE(bench.keysNow.empty());
}
