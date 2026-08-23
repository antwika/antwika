#include <gtest/gtest.h>

#include <optional>

#include <antwika/input/ExtraModifiers.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/input/KeyModifiers.hpp>

#include "antwika/editor/ui/EditorBindings.hpp"

using antwika::editor::Action;
using antwika::editor::getActionKey;
using antwika::editor::actionMapFrom;
using antwika::editor::Chord;
using antwika::editor::getDefaultChords;
using antwika::editor::getHeldAction;
using antwika::editor::KeyBindings;
using antwika::editor::getShiftedAction;
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
                getActionKey(Action::Save),
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
                getActionKey(Action::Save), Key::S, KeyModifiers{}));
    }

    TEST(EditorBindingsTest, ActionMapFrom_RefusesAModifierTheChordDoesNotName)
    {
        KeyBindings keyBindings;
        keyBindings[Action::Save] = Chord{.key = Key::S, .ctrl = true};

        const auto actions = actionMapFrom(keyBindings);

        EXPECT_FALSE(
            actions.matches(
                getActionKey(Action::Save),
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
                getActionKey(Action::ToolBrush), Key::B, KeyModifiers{}));
        EXPECT_FALSE(
            actions.matches(
                getActionKey(Action::ToolBrush),
                Key::B,
                KeyModifiers{.control = true}));
    }

    TEST(EditorBindingsTest, ActionMapFrom_CarriesNothingForAnUnboundAction)
    {
        KeyBindings keyBindings;
        keyBindings[Action::ToolLamp] = std::nullopt;

        const auto actions = actionMapFrom(keyBindings);

        EXPECT_FALSE(actions.isBound(getActionKey(Action::ToolLamp)));
        EXPECT_FALSE(
            actions.matches(
                getActionKey(Action::ToolLamp), Key::L, KeyModifiers{}));
    }

    TEST(EditorBindingsTest, HeldAction_TakesTheKeyWhateverIsHeldBeside)
    {
        KeyBindings keyBindings;
        keyBindings[Action::WalkNorth] = Chord{.key = Key::W};

        const auto actions = actionMapFrom(keyBindings);

        EXPECT_TRUE(
            actions.matches(
                getHeldAction(Action::WalkNorth), Key::W, KeyModifiers{}));
        EXPECT_TRUE(
            actions.matches(
                getHeldAction(Action::WalkNorth),
                Key::W,
                KeyModifiers{.shift = true}));
        EXPECT_FALSE(
            actions.matches(
                getHeldAction(Action::WalkNorth), Key::A, KeyModifiers{}));
    }

    TEST(EditorBindingsTest, ShiftedAction_TakesThePlainChordUnderShiftAlone)
    {
        KeyBindings keyBindings;
        keyBindings[Action::KindStone] = Chord{.key = Key::N};

        const auto actions = actionMapFrom(keyBindings);

        EXPECT_TRUE(
            actions.matches(
                getShiftedAction(Action::KindStone),
                Key::N,
                KeyModifiers{.shift = true}));
        EXPECT_FALSE(
            actions.matches(
                getShiftedAction(Action::KindStone), Key::N, KeyModifiers{}));
        EXPECT_FALSE(
            actions.matches(
                getShiftedAction(Action::KindStone),
                Key::N,
                KeyModifiers{.shift = true, .control = true}));
    }

    TEST(EditorBindingsTest, ShiftedAction_CarriesNothingForAChordUnderShift)
    {
        KeyBindings keyBindings;
        keyBindings[Action::Redo] =
            Chord{.key = Key::Z, .ctrl = true, .shift = true};

        const auto actions = actionMapFrom(keyBindings);

        EXPECT_FALSE(actions.isBound(getShiftedAction(Action::Redo)));
    }

    TEST(EditorBindingsTest, ActionMapFrom_BindsEveryChordTheDefaultsName)
    {
        const auto keyBindings = getDefaultChords();
        const auto actions = actionMapFrom(keyBindings);

        for (const auto &[action, chord] : keyBindings)
        {
            EXPECT_EQ(actions.isBound(getActionKey(action)), chord.has_value())
                << getActionKey(action);
        }
    }

}
