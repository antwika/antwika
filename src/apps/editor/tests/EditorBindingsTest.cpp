#include <gtest/gtest.h>

#include <optional>

#include <antwika/input/ExtraModifiers.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/input/KeyModifiers.hpp>

#include "antwika/editor/ui/EditorBindings.hpp"

using antwika::editor::Action;
using antwika::editor::actionKey;
using antwika::editor::actionMapFrom;
using antwika::editor::Chord;
using antwika::editor::defaultChords;
using antwika::editor::heldAction;
using antwika::editor::KeyBindings;
using antwika::editor::shiftedAction;
using antwika::input::Key;
using antwika::input::KeyModifiers;

namespace
{

    TEST(EditorBindingsTest, ActionMapFrom_TakesAChordWithTheModifiersItNames)
    {
        KeyBindings keyBindings;
        keyBindings[Action::Save] = Chord{.key = Key::S, .ctrl = true};

        const auto actions = actionMapFrom(keyBindings);

        EXPECT_TRUE(
            actions.matches(
                actionKey(Action::Save),
                Key::S,
                KeyModifiers{.control = true}));
    }

    TEST(EditorBindingsTest, ActionMapFrom_RefusesAChordWithoutItsModifiers)
    {
        KeyBindings keyBindings;
        keyBindings[Action::Save] = Chord{.key = Key::S, .ctrl = true};

        const auto actions = actionMapFrom(keyBindings);

        EXPECT_FALSE(
            actions.matches(
                actionKey(Action::Save), Key::S, KeyModifiers{}));
    }

    TEST(EditorBindingsTest, ActionMapFrom_RefusesAModifierTheChordDoesNotName)
    {
        KeyBindings keyBindings;
        keyBindings[Action::Save] = Chord{.key = Key::S, .ctrl = true};

        const auto actions = actionMapFrom(keyBindings);

        EXPECT_FALSE(
            actions.matches(
                actionKey(Action::Save),
                Key::S,
                KeyModifiers{.shift = true, .control = true}));
    }

    TEST(EditorBindingsTest, ActionMapFrom_RefusesAModifierOnAPlainChord)
    {
        KeyBindings keyBindings;
        keyBindings[Action::ToolBrush] = Chord{.key = Key::B};

        const auto actions = actionMapFrom(keyBindings);

        EXPECT_TRUE(
            actions.matches(
                actionKey(Action::ToolBrush), Key::B, KeyModifiers{}));
        EXPECT_FALSE(
            actions.matches(
                actionKey(Action::ToolBrush),
                Key::B,
                KeyModifiers{.control = true}));
    }

    TEST(EditorBindingsTest, ActionMapFrom_CarriesNothingForAnUnboundAction)
    {
        KeyBindings keyBindings;
        keyBindings[Action::ToolLamp] = std::nullopt;

        const auto actions = actionMapFrom(keyBindings);

        EXPECT_FALSE(actions.isBound(actionKey(Action::ToolLamp)));
        EXPECT_FALSE(
            actions.matches(
                actionKey(Action::ToolLamp), Key::L, KeyModifiers{}));
    }

    TEST(EditorBindingsTest, HeldAction_TakesTheKeyWhateverIsHeldBeside)
    {
        KeyBindings keyBindings;
        keyBindings[Action::WalkNorth] = Chord{.key = Key::W};

        const auto actions = actionMapFrom(keyBindings);

        EXPECT_TRUE(
            actions.matches(
                heldAction(Action::WalkNorth), Key::W, KeyModifiers{}));
        EXPECT_TRUE(
            actions.matches(
                heldAction(Action::WalkNorth),
                Key::W,
                KeyModifiers{.shift = true}));
        EXPECT_FALSE(
            actions.matches(
                heldAction(Action::WalkNorth), Key::A, KeyModifiers{}));
    }

    TEST(EditorBindingsTest, ShiftedAction_TakesThePlainChordUnderShiftAlone)
    {
        KeyBindings keyBindings;
        keyBindings[Action::KindStone] = Chord{.key = Key::N};

        const auto actions = actionMapFrom(keyBindings);

        EXPECT_TRUE(
            actions.matches(
                shiftedAction(Action::KindStone),
                Key::N,
                KeyModifiers{.shift = true}));
        EXPECT_FALSE(
            actions.matches(
                shiftedAction(Action::KindStone), Key::N, KeyModifiers{}));
        EXPECT_FALSE(
            actions.matches(
                shiftedAction(Action::KindStone),
                Key::N,
                KeyModifiers{.shift = true, .control = true}));
    }

    TEST(EditorBindingsTest, ShiftedAction_CarriesNothingForAChordUnderShift)
    {
        KeyBindings keyBindings;
        keyBindings[Action::Redo] =
            Chord{.key = Key::Z, .ctrl = true, .shift = true};

        const auto actions = actionMapFrom(keyBindings);

        EXPECT_FALSE(actions.isBound(shiftedAction(Action::Redo)));
    }

    TEST(EditorBindingsTest, ActionMapFrom_BindsEveryChordTheDefaultsName)
    {
        const auto keyBindings = defaultChords();
        const auto actions = actionMapFrom(keyBindings);

        for (const auto &[action, chord] : keyBindings)
        {
            EXPECT_EQ(actions.isBound(actionKey(action)), chord.has_value())
                << actionKey(action);
        }
    }

}
