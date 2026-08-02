#include "antwika/music_editor/EditorKeys.hpp"

#include <string>

#include <gtest/gtest.h>

#include <antwika/input/Key.hpp>
#include <antwika/ui/Keyboard.hpp>

using antwika::input::Key;
using antwika::music_editor::typedCharacterFor;
using antwika::music_editor::uiKeyFor;

TEST(EditorKeysTest, TellsTheUiWhatItActsOn)
{
    EXPECT_EQ(uiKeyFor(Key::Tab, false), antwika::ui::Key::FocusNext);
    EXPECT_EQ(uiKeyFor(Key::Tab, true), antwika::ui::Key::FocusPrevious);
    EXPECT_EQ(uiKeyFor(Key::Enter, false), antwika::ui::Key::Activate);
    EXPECT_EQ(
        uiKeyFor(Key::Backspace, false), antwika::ui::Key::Backspace);
    EXPECT_EQ(uiKeyFor(Key::ArrowLeft, false), antwika::ui::Key::MoveLeft);
    EXPECT_EQ(
        uiKeyFor(Key::ArrowRight, false), antwika::ui::Key::MoveRight);
}

// A field that gave up would throw away the line being written.
// This editor has nothing else Escape could mean.
TEST(EditorKeysTest, EscapeIsNotCancelHere)
{
    EXPECT_FALSE(uiKeyFor(Key::Escape, false).has_value());
}

TEST(EditorKeysTest, AKeyTheUiHasNoMeaningForSaysNothing)
{
    EXPECT_FALSE(uiKeyFor(Key::A, false).has_value());
    EXPECT_FALSE(uiKeyFor(Key::F1, false).has_value());
}

TEST(EditorKeysTest, TypesLettersInBothCases)
{
    EXPECT_EQ(typedCharacterFor(Key::A, false), 'a');
    EXPECT_EQ(typedCharacterFor(Key::Z, false), 'z');
    EXPECT_EQ(typedCharacterFor(Key::A, true), 'A');
    EXPECT_EQ(typedCharacterFor(Key::Z, true), 'Z');
}

TEST(EditorKeysTest, TypesDigits)
{
    EXPECT_EQ(typedCharacterFor(Key::Digit0, false), '0');
    EXPECT_EQ(typedCharacterFor(Key::Digit9, false), '9');
}

// The one requirement this table has that a showcase's does not.
// A layout missing one would make a form of the grammar untypeable.
TEST(EditorKeysTest, EveryCharacterTheNotationUsesIsReachable)
{
    std::string reachable;

    reachable.push_back(typedCharacterFor(Key::LeftBracket, false));
    reachable.push_back(typedCharacterFor(Key::RightBracket, false));
    reachable.push_back(typedCharacterFor(Key::Comma, true));
    reachable.push_back(typedCharacterFor(Key::Period, true));
    reachable.push_back(typedCharacterFor(Key::Digit8, true));
    reachable.push_back(typedCharacterFor(Key::Slash, false));
    reachable.push_back(typedCharacterFor(Key::Digit5, true));
    reachable.push_back(typedCharacterFor(Key::Digit1, true));
    reachable.push_back(typedCharacterFor(Key::Slash, true));
    reachable.push_back(typedCharacterFor(Key::Digit9, true));
    reachable.push_back(typedCharacterFor(Key::Digit0, true));
    reachable.push_back(typedCharacterFor(Key::Comma, false));
    reachable.push_back(typedCharacterFor(Key::Grave, true));
    reachable.push_back(typedCharacterFor(Key::Minus, false));
    reachable.push_back(typedCharacterFor(Key::Space, false));

    EXPECT_EQ(reachable, "[]<>*/%!?(),~- ");
}

TEST(EditorKeysTest, TypesTheRestOfItsRowsToo)
{
    EXPECT_EQ(typedCharacterFor(Key::Digit2, true), '@');
    EXPECT_EQ(typedCharacterFor(Key::Digit3, true), '#');
    EXPECT_EQ(typedCharacterFor(Key::Digit4, true), '$');
    EXPECT_EQ(typedCharacterFor(Key::Digit6, true), '^');
    EXPECT_EQ(typedCharacterFor(Key::Digit7, true), '&');
    EXPECT_EQ(typedCharacterFor(Key::Minus, true), '_');
    EXPECT_EQ(typedCharacterFor(Key::Period, false), '.');
    EXPECT_EQ(typedCharacterFor(Key::LeftBracket, true), '{');
    EXPECT_EQ(typedCharacterFor(Key::RightBracket, true), '}');
    EXPECT_EQ(typedCharacterFor(Key::Grave, false), '`');
    EXPECT_EQ(typedCharacterFor(Key::Equal, false), '=');
    EXPECT_EQ(typedCharacterFor(Key::Equal, true), '+');
}

TEST(EditorKeysTest, AKeyThatTypesNothingSaysSo)
{
    EXPECT_EQ(typedCharacterFor(Key::F1, false), '\0');
    EXPECT_EQ(typedCharacterFor(Key::Escape, false), '\0');
    EXPECT_EQ(typedCharacterFor(Key::LeftShift, false), '\0');
}
