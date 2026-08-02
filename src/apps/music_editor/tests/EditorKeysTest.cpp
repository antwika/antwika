#include "antwika/music_editor/EditorKeys.hpp"

#include <string>

#include <gtest/gtest.h>

#include <antwika/input/Key.hpp>
#include <antwika/input/KeyModifiers.hpp>
#include <antwika/ui/Keyboard.hpp>

using antwika::input::Key;
using antwika::input::KeyModifiers;
using antwika::music_editor::KeyLayout;
using antwika::music_editor::nameOf;
using antwika::music_editor::typedTextFor;
using antwika::music_editor::uiKeyFor;

namespace
{
    constexpr KeyModifiers kNone{};
    constexpr KeyModifiers kShift{.shift = true};
    constexpr KeyModifiers kAlt{.alt = true};
    constexpr KeyModifiers kControl{.control = true};

    // A std::string, so a failure prints the characters.
    // Two views would print their addresses instead.
    [[nodiscard]] std::string swedish(
        const Key key, const KeyModifiers modifiers = kNone)
    {
        return std::string{
            typedTextFor(key, modifiers, KeyLayout::Swedish)};
    }

    [[nodiscard]] std::string english(
        const Key key, const KeyModifiers modifiers = kNone)
    {
        return std::string{
            typedTextFor(key, modifiers, KeyLayout::English)};
    }
} // namespace

TEST(EditorKeysTest, TellsTheUiWhatItActsOn)
{
    EXPECT_EQ(uiKeyFor(Key::Enter, kNone), antwika::ui::Key::Activate);
    EXPECT_EQ(
        uiKeyFor(Key::Backspace, kNone), antwika::ui::Key::Backspace);
    EXPECT_EQ(uiKeyFor(Key::Delete, kNone), antwika::ui::Key::Delete);
    EXPECT_EQ(
        uiKeyFor(Key::ArrowLeft, kNone), antwika::ui::Key::MoveLeft);
    EXPECT_EQ(
        uiKeyFor(Key::ArrowRight, kNone), antwika::ui::Key::MoveRight);
    EXPECT_EQ(uiKeyFor(Key::ArrowUp, kNone), antwika::ui::Key::MoveUp);
    EXPECT_EQ(
        uiKeyFor(Key::ArrowDown, kNone), antwika::ui::Key::MoveDown);
    EXPECT_EQ(
        uiKeyFor(Key::Home, kNone), antwika::ui::Key::MoveLineStart);
    EXPECT_EQ(uiKeyFor(Key::End, kNone), antwika::ui::Key::MoveLineEnd);
}

// The caret moves are the one place shift changes what the UI is told.
// Everywhere else it is read where the characters are.
TEST(EditorKeysTest, ShiftTurnsAnArrowIntoTheSelectingOneBesideIt)
{
    EXPECT_EQ(
        uiKeyFor(Key::ArrowLeft, kShift), antwika::ui::Key::SelectLeft);
    EXPECT_EQ(
        uiKeyFor(Key::ArrowRight, kShift),
        antwika::ui::Key::SelectRight);
    EXPECT_EQ(
        uiKeyFor(Key::ArrowUp, kShift), antwika::ui::Key::SelectUp);
    EXPECT_EQ(
        uiKeyFor(Key::ArrowDown, kShift), antwika::ui::Key::SelectDown);
    EXPECT_EQ(
        uiKeyFor(Key::Home, kShift),
        antwika::ui::Key::SelectLineStart);
    EXPECT_EQ(
        uiKeyFor(Key::End, kShift), antwika::ui::Key::SelectLineEnd);

    EXPECT_EQ(uiKeyFor(Key::Enter, kShift), uiKeyFor(Key::Enter, kNone));
}

TEST(EditorKeysTest, ControlIsACopyAndACut)
{
    EXPECT_EQ(uiKeyFor(Key::C, kControl), antwika::ui::Key::Copy);
    EXPECT_EQ(uiKeyFor(Key::X, kControl), antwika::ui::Key::Cut);
}

// A paste is the clipboard's characters.
// This table has no clipboard, so the sink types them.
TEST(EditorKeysTest, ControlAndVIsNotThisTablesToAnswer)
{
    EXPECT_FALSE(uiKeyFor(Key::V, kControl).has_value());
    EXPECT_FALSE(uiKeyFor(Key::Enter, kControl).has_value());
}

// Or a copy would leave a c behind it in the document.
TEST(EditorKeysTest, HoldingControlTypesNothingAtAll)
{
    EXPECT_TRUE(swedish(Key::C, kControl).empty());
    EXPECT_TRUE(english(Key::V, kControl).empty());
    EXPECT_TRUE(swedish(Key::Space, kControl).empty());
    EXPECT_TRUE(swedish(Key::Digit8, kControl).empty());
}

// A field that gave up would throw away the score being written.
// Escape pauses instead, and the sink reads that key itself.
TEST(EditorKeysTest, EscapeIsNotCancelHere)
{
    EXPECT_FALSE(uiKeyFor(Key::Escape, kNone).has_value());
    EXPECT_FALSE(uiKeyFor(Key::Escape, kShift).has_value());
}

// There is one thing to type into, so Tab has no focus to move.
// It indents, which makes it characters rather than a meaning.
TEST(EditorKeysTest, TabIsNotFocusHere)
{
    EXPECT_FALSE(uiKeyFor(Key::Tab, kNone).has_value());
    EXPECT_FALSE(uiKeyFor(Key::Tab, kShift).has_value());
    EXPECT_EQ(swedish(Key::Tab), "  ");
    EXPECT_EQ(english(Key::Tab, kShift), "  ");
}

TEST(EditorKeysTest, AKeyTheUiHasNoMeaningForSaysNothing)
{
    EXPECT_FALSE(uiKeyFor(Key::A, kNone).has_value());
    EXPECT_FALSE(uiKeyFor(Key::F10, kNone).has_value());
    EXPECT_FALSE(uiKeyFor(Key::Digit0, kNone).has_value());
}

// The one run of keys both boards agree about.
TEST(EditorKeysTest, TypesLettersInBothCasesOnEitherBoard)
{
    EXPECT_EQ(swedish(Key::A), "a");
    EXPECT_EQ(swedish(Key::Z, kShift), "Z");
    EXPECT_EQ(english(Key::A), "a");
    EXPECT_EQ(english(Key::Z, kShift), "Z");
    EXPECT_EQ(swedish(Key::M), "m");
}

TEST(EditorKeysTest, TypesDigitsAtBothEndsOfTheirRow)
{
    EXPECT_EQ(swedish(Key::Digit0), "0");
    EXPECT_EQ(swedish(Key::Digit9), "9");
    EXPECT_EQ(english(Key::Digit0), "0");
    EXPECT_EQ(english(Key::Digit4), "4");
}

// The one requirement this table has that a showcase's does not.
// A layout missing one would make a form of the grammar untypeable.
TEST(EditorKeysTest, TheSwedishBoardReachesEveryCharacterTheNotationUses)
{
    std::string reachable;

    reachable += swedish(Key::Digit8, kAlt);
    reachable += swedish(Key::Digit9, kAlt);
    reachable += swedish(Key::IntlBackslash);
    reachable += swedish(Key::IntlBackslash, kShift);
    reachable += swedish(Key::Backslash, kShift);
    reachable += swedish(Key::Digit7, kShift);
    reachable += swedish(Key::Digit5, kShift);
    reachable += swedish(Key::Digit1, kShift);
    reachable += swedish(Key::Minus, kShift);
    reachable += swedish(Key::Digit8, kShift);
    reachable += swedish(Key::Digit9, kShift);
    reachable += swedish(Key::Comma);
    reachable += swedish(Key::RightBracket, kAlt);
    reachable += swedish(Key::Slash);
    reachable += swedish(Key::Space);

    EXPECT_EQ(reachable, "[]<>*/%!?(),~- ");
}

TEST(EditorKeysTest, TheEnglishBoardReachesThemToo)
{
    std::string reachable;

    reachable += english(Key::LeftBracket);
    reachable += english(Key::RightBracket);
    reachable += english(Key::Comma, kShift);
    reachable += english(Key::Period, kShift);
    reachable += english(Key::Digit8, kShift);
    reachable += english(Key::Slash);
    reachable += english(Key::Digit5, kShift);
    reachable += english(Key::Digit1, kShift);
    reachable += english(Key::Slash, kShift);
    reachable += english(Key::Digit9, kShift);
    reachable += english(Key::Digit0, kShift);
    reachable += english(Key::Comma);
    reachable += english(Key::Grave, kShift);
    reachable += english(Key::Minus);
    reachable += english(Key::Space);

    EXPECT_EQ(reachable, "[]<>*/%!?(),~- ");
}

// A voice line opens with these two.
// Where they are is most of why there is a Swedish table at all.
TEST(EditorKeysTest, TypesWhatAVoiceLineOpensWithOnEitherBoard)
{
    EXPECT_EQ(
        swedish(Key::Digit4, kAlt) + swedish(Key::Period, kShift), "$:");

    EXPECT_EQ(
        english(Key::Digit4, kShift)
            + english(Key::Semicolon, kShift),
        "$:");
}

// The same key, two boards, two characters.
// Shift and the full stop is a colon on one and an angle on the other.
TEST(EditorKeysTest, TheTwoBoardsDisagreeAboutTheSameKey)
{
    EXPECT_EQ(swedish(Key::Period, kShift), ":");
    EXPECT_EQ(english(Key::Period, kShift), ">");

    EXPECT_EQ(swedish(Key::Digit2, kShift), "\"");
    EXPECT_EQ(english(Key::Digit2, kShift), "@");

    EXPECT_EQ(swedish(Key::Minus), "+");
    EXPECT_EQ(english(Key::Minus), "-");
}

TEST(EditorKeysTest, TypesTheRestOfTheEnglishRowsToo)
{
    EXPECT_EQ(english(Key::Digit3, kShift), "#");
    EXPECT_EQ(english(Key::Digit6, kShift), "^");
    EXPECT_EQ(english(Key::Digit7, kShift), "&");
    EXPECT_EQ(english(Key::Minus, kShift), "_");
    EXPECT_EQ(english(Key::Period), ".");
    EXPECT_EQ(english(Key::LeftBracket, kShift), "{");
    EXPECT_EQ(english(Key::RightBracket, kShift), "}");
    EXPECT_EQ(english(Key::Semicolon), ";");
    EXPECT_EQ(english(Key::Apostrophe), "'");
    EXPECT_EQ(english(Key::Apostrophe, kShift), "\"");
    EXPECT_EQ(english(Key::Grave), "`");
    EXPECT_EQ(english(Key::Equal), "=");
    EXPECT_EQ(english(Key::Equal, kShift), "+");
    EXPECT_EQ(english(Key::Backslash), "\\");
    EXPECT_EQ(english(Key::Backslash, kShift), "|");
    EXPECT_EQ(english(Key::IntlBackslash), "\\");
}

TEST(EditorKeysTest, TypesTheRestOfTheSwedishRowsToo)
{
    EXPECT_EQ(swedish(Key::Digit0, kShift), "=");
    EXPECT_EQ(swedish(Key::Digit3, kShift), "#");
    EXPECT_EQ(swedish(Key::Digit6, kShift), "&");
    EXPECT_EQ(swedish(Key::Digit7, kAlt), "{");
    EXPECT_EQ(swedish(Key::Digit0, kAlt), "}");
    EXPECT_EQ(swedish(Key::Digit2, kAlt), "@");
    EXPECT_EQ(swedish(Key::Minus, kAlt), "\\");
    EXPECT_EQ(swedish(Key::Backslash), "'");
    EXPECT_EQ(swedish(Key::RightBracket, kShift), "^");
    EXPECT_EQ(swedish(Key::Comma, kShift), ";");
    EXPECT_EQ(swedish(Key::Period), ".");
    EXPECT_EQ(swedish(Key::Slash, kShift), "_");
    EXPECT_EQ(swedish(Key::IntlBackslash, kAlt), "|");
}

// antwika::gfx covers printable ASCII.
// And a score is not written in a ring-a in any case.
TEST(EditorKeysTest, TheSwedishLettersThisWindowCannotDrawTypeNothing)
{
    EXPECT_TRUE(swedish(Key::LeftBracket).empty());
    EXPECT_TRUE(swedish(Key::Semicolon).empty());
    EXPECT_TRUE(swedish(Key::Apostrophe, kShift).empty());
    EXPECT_TRUE(swedish(Key::Grave).empty());
    EXPECT_TRUE(swedish(Key::Equal, kShift).empty());
    EXPECT_TRUE(swedish(Key::RightBracket).empty());
    EXPECT_TRUE(swedish(Key::Digit4, kShift).empty());
}

// Which is what a board with nothing on a key's third level does.
TEST(EditorKeysTest, AltOnAKeyWithNothingOnItTypesWhatTheKeyTypes)
{
    EXPECT_EQ(swedish(Key::Digit5, kAlt), "5");
    EXPECT_EQ(english(Key::Digit5, kAlt), "5");
    EXPECT_EQ(english(Key::A, kAlt), "a");
}

TEST(EditorKeysTest, AKeyThatTypesNothingSaysSo)
{
    EXPECT_TRUE(swedish(Key::F1).empty());
    EXPECT_TRUE(swedish(Key::Escape).empty());
    EXPECT_TRUE(english(Key::LeftShift).empty());
    EXPECT_TRUE(english(Key::Enter).empty());
    EXPECT_TRUE(english(Key::ArrowUp, kShift).empty());
}

TEST(EditorKeysTest, NamesEachLayout)
{
    EXPECT_EQ(nameOf(KeyLayout::Swedish), "swedish");
    EXPECT_EQ(nameOf(KeyLayout::English), "english");
}
