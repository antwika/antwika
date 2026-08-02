#include "antwika/music_editor/EditorKeys.hpp"

#include <string>

#include <gtest/gtest.h>

#include <antwika/input/Key.hpp>
#include <antwika/ui/Keyboard.hpp>

using antwika::input::Key;
using antwika::music_editor::typedTextFor;
using antwika::music_editor::uiKeyFor;

TEST(EditorKeysTest, TellsTheUiWhatItActsOn)
{
    EXPECT_EQ(uiKeyFor(Key::Enter, false), antwika::ui::Key::Activate);
    EXPECT_EQ(
        uiKeyFor(Key::Backspace, false), antwika::ui::Key::Backspace);
    EXPECT_EQ(
        uiKeyFor(Key::ArrowLeft, false), antwika::ui::Key::MoveLeft);
    EXPECT_EQ(
        uiKeyFor(Key::ArrowRight, false), antwika::ui::Key::MoveRight);
    EXPECT_EQ(uiKeyFor(Key::ArrowUp, false), antwika::ui::Key::MoveUp);
    EXPECT_EQ(
        uiKeyFor(Key::ArrowDown, false), antwika::ui::Key::MoveDown);
}

// Shift changes no meaning here; it is read where the characters are.
TEST(EditorKeysTest, ShiftChangesNothingItTellsTheUi)
{
    EXPECT_EQ(uiKeyFor(Key::Enter, true), uiKeyFor(Key::Enter, false));
    EXPECT_EQ(
        uiKeyFor(Key::ArrowUp, true), uiKeyFor(Key::ArrowUp, false));
}

// A field that gave up would throw away the score being written.
// Escape pauses instead, and the sink reads that key itself.
TEST(EditorKeysTest, EscapeIsNotCancelHere)
{
    EXPECT_FALSE(uiKeyFor(Key::Escape, false).has_value());
    EXPECT_FALSE(uiKeyFor(Key::Escape, true).has_value());
}

// There is one thing to type into, so Tab has no focus to move.
// It indents, which makes it characters rather than a meaning.
TEST(EditorKeysTest, TabIsNotFocusHere)
{
    EXPECT_FALSE(uiKeyFor(Key::Tab, false).has_value());
    EXPECT_FALSE(uiKeyFor(Key::Tab, true).has_value());
    EXPECT_EQ(typedTextFor(Key::Tab, false), "  ");
    EXPECT_EQ(typedTextFor(Key::Tab, true), "  ");
}

TEST(EditorKeysTest, AKeyTheUiHasNoMeaningForSaysNothing)
{
    EXPECT_FALSE(uiKeyFor(Key::A, false).has_value());
    EXPECT_FALSE(uiKeyFor(Key::F1, false).has_value());
    EXPECT_FALSE(uiKeyFor(Key::Digit0, false).has_value());
}

TEST(EditorKeysTest, TypesLettersInBothCases)
{
    EXPECT_EQ(typedTextFor(Key::A, false), "a");
    EXPECT_EQ(typedTextFor(Key::Z, false), "z");
    EXPECT_EQ(typedTextFor(Key::A, true), "A");
    EXPECT_EQ(typedTextFor(Key::Z, true), "Z");
    EXPECT_EQ(typedTextFor(Key::M, false), "m");
}

TEST(EditorKeysTest, TypesDigitsAtBothEndsOfTheirRow)
{
    EXPECT_EQ(typedTextFor(Key::Digit0, false), "0");
    EXPECT_EQ(typedTextFor(Key::Digit9, false), "9");
    EXPECT_EQ(typedTextFor(Key::Digit4, false), "4");
}

// The one requirement this table has that a showcase's does not.
// A layout missing one would make a form of the grammar untypeable.
TEST(EditorKeysTest, EveryCharacterTheNotationUsesIsReachable)
{
    std::string reachable;

    reachable += typedTextFor(Key::LeftBracket, false);
    reachable += typedTextFor(Key::RightBracket, false);
    reachable += typedTextFor(Key::Comma, true);
    reachable += typedTextFor(Key::Period, true);
    reachable += typedTextFor(Key::Digit8, true);
    reachable += typedTextFor(Key::Slash, false);
    reachable += typedTextFor(Key::Digit5, true);
    reachable += typedTextFor(Key::Digit1, true);
    reachable += typedTextFor(Key::Slash, true);
    reachable += typedTextFor(Key::Digit9, true);
    reachable += typedTextFor(Key::Digit0, true);
    reachable += typedTextFor(Key::Comma, false);
    reachable += typedTextFor(Key::Grave, true);
    reachable += typedTextFor(Key::Minus, false);
    reachable += typedTextFor(Key::Space, false);

    EXPECT_EQ(reachable, "[]<>*/%!?(),~- ");
}

// A voice line opens with these two.
// A document is untypeable without them.
TEST(EditorKeysTest, TypesWhatAVoiceLineOpensWith)
{
    std::string mark;

    mark += typedTextFor(Key::Digit4, true);
    mark += typedTextFor(Key::Semicolon, true);

    EXPECT_EQ(mark, "$:");
}

TEST(EditorKeysTest, TypesTheRestOfItsRowsToo)
{
    EXPECT_EQ(typedTextFor(Key::Digit2, true), "@");
    EXPECT_EQ(typedTextFor(Key::Digit3, true), "#");
    EXPECT_EQ(typedTextFor(Key::Digit6, true), "^");
    EXPECT_EQ(typedTextFor(Key::Digit7, true), "&");
    EXPECT_EQ(typedTextFor(Key::Minus, true), "_");
    EXPECT_EQ(typedTextFor(Key::Period, false), ".");
    EXPECT_EQ(typedTextFor(Key::LeftBracket, true), "{");
    EXPECT_EQ(typedTextFor(Key::RightBracket, true), "}");
    EXPECT_EQ(typedTextFor(Key::Semicolon, false), ";");
    EXPECT_EQ(typedTextFor(Key::Apostrophe, false), "'");
    EXPECT_EQ(typedTextFor(Key::Apostrophe, true), "\"");
    EXPECT_EQ(typedTextFor(Key::Grave, false), "`");
    EXPECT_EQ(typedTextFor(Key::Equal, false), "=");
    EXPECT_EQ(typedTextFor(Key::Equal, true), "+");
}

TEST(EditorKeysTest, AKeyThatTypesNothingSaysSo)
{
    EXPECT_TRUE(typedTextFor(Key::F1, false).empty());
    EXPECT_TRUE(typedTextFor(Key::Escape, false).empty());
    EXPECT_TRUE(typedTextFor(Key::LeftShift, false).empty());
    EXPECT_TRUE(typedTextFor(Key::Backslash, true).empty());
    EXPECT_TRUE(typedTextFor(Key::Enter, false).empty());
    EXPECT_TRUE(typedTextFor(Key::ArrowUp, false).empty());
}
