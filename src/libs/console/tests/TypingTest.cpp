#include <gtest/gtest.h>

#include "antwika/console/Typing.hpp"

using antwika::console::consoleKeyFor;
using antwika::console::KeyboardLayout;
using antwika::console::typedCharacterFor;
using antwika::input::Key;
using UiKey = antwika::ui::Key;

TEST(TypingTest, ConsoleKeyFor_NamesTheKeysTheFieldActsOn)
{
    EXPECT_EQ(consoleKeyFor(Key::Backspace), UiKey::Backspace);
    EXPECT_EQ(consoleKeyFor(Key::ArrowLeft), UiKey::MoveLeft);
    EXPECT_EQ(consoleKeyFor(Key::ArrowRight), UiKey::MoveRight);
}

TEST(TypingTest, ConsoleKeyFor_HasNoMeaningForAnythingElse)
{
    EXPECT_FALSE(consoleKeyFor(Key::Enter).has_value());
    EXPECT_FALSE(consoleKeyFor(Key::Tab).has_value());
    EXPECT_FALSE(consoleKeyFor(Key::A).has_value());
}

TEST(TypingTest, TypedCharacterFor_TypesByTheBoardItIsHanded)
{
    EXPECT_EQ(
        typedCharacterFor(Key::Slash, true, KeyboardLayout::Swedish),
        '_');
    EXPECT_EQ(
        typedCharacterFor(Key::Slash, true, KeyboardLayout::English),
        '\0');
    EXPECT_EQ(
        typedCharacterFor(Key::Minus, true, KeyboardLayout::English),
        '_');
}

TEST(TypingTest, TypedCharacterFor_SpellsEveryDigitOnEitherBoard)
{
    EXPECT_EQ(
        typedCharacterFor(Key::Digit0, false, KeyboardLayout::English),
        '0');
    EXPECT_EQ(
        typedCharacterFor(Key::Digit9, false, KeyboardLayout::English),
        '9');
    EXPECT_EQ(
        typedCharacterFor(Key::Digit9, false, KeyboardLayout::Swedish),
        '9');
}

TEST(TypingTest, TypedCharacterFor_ShiftsADigitOnlyOnTheSwedishBoard)
{
    EXPECT_EQ(
        typedCharacterFor(Key::Digit0, true, KeyboardLayout::Swedish),
        '=');
    EXPECT_EQ(
        typedCharacterFor(Key::Digit9, true, KeyboardLayout::Swedish),
        ')');
    EXPECT_EQ(
        typedCharacterFor(Key::Digit4, true, KeyboardLayout::Swedish),
        '\0');
    EXPECT_EQ(
        typedCharacterFor(Key::Digit9, true, KeyboardLayout::English),
        '\0');
}

TEST(TypingTest, TypedCharacterFor_CountsALetterUpwardsFromA)
{
    EXPECT_EQ(
        typedCharacterFor(Key::A, false, KeyboardLayout::English), 'a');
    EXPECT_EQ(
        typedCharacterFor(Key::Z, false, KeyboardLayout::English), 'z');
    EXPECT_EQ(
        typedCharacterFor(Key::A, true, KeyboardLayout::English), 'A');
    EXPECT_EQ(
        typedCharacterFor(Key::Z, true, KeyboardLayout::Swedish), 'Z');
}

TEST(TypingTest, TypedCharacterFor_TypesASpaceForTheSpaceBar)
{
    EXPECT_EQ(
        typedCharacterFor(Key::Space, false, KeyboardLayout::English),
        ' ');
    EXPECT_EQ(
        typedCharacterFor(Key::Space, true, KeyboardLayout::Swedish),
        ' ');
}

TEST(TypingTest, TypedCharacterFor_TypesEveryCharacterJsonNeeds)
{
    constexpr auto kEnglish = KeyboardLayout::English;

    EXPECT_EQ(typedCharacterFor(Key::LeftBracket, true, kEnglish), '{');
    EXPECT_EQ(typedCharacterFor(Key::RightBracket, true, kEnglish), '}');
    EXPECT_EQ(typedCharacterFor(Key::LeftBracket, false, kEnglish), '[');
    EXPECT_EQ(
        typedCharacterFor(Key::RightBracket, false, kEnglish), ']');
    EXPECT_EQ(typedCharacterFor(Key::Apostrophe, true, kEnglish), '"');
    EXPECT_EQ(typedCharacterFor(Key::Apostrophe, false, kEnglish), '\'');
    EXPECT_EQ(typedCharacterFor(Key::Semicolon, true, kEnglish), ':');
    EXPECT_EQ(typedCharacterFor(Key::Semicolon, false, kEnglish), ';');
    EXPECT_EQ(typedCharacterFor(Key::Comma, false, kEnglish), ',');
    EXPECT_EQ(typedCharacterFor(Key::Comma, true, kEnglish), '\0');
}

TEST(TypingTest, TypedCharacterFor_TypesAFileNameOnTheEnglishBoard)
{
    constexpr auto kEnglish = KeyboardLayout::English;

    EXPECT_EQ(typedCharacterFor(Key::Minus, false, kEnglish), '-');
    EXPECT_EQ(typedCharacterFor(Key::Period, false, kEnglish), '.');
    EXPECT_EQ(typedCharacterFor(Key::Period, true, kEnglish), '\0');
}

TEST(TypingTest, TypedCharacterFor_ReachesABraceThroughAltOnTheSwedishBoard)
{
    constexpr auto kSwedish = KeyboardLayout::Swedish;

    EXPECT_EQ(typedCharacterFor(Key::Digit7, false, kSwedish, true), '{');
    EXPECT_EQ(typedCharacterFor(Key::Digit0, false, kSwedish, true), '}');
    EXPECT_EQ(typedCharacterFor(Key::Digit8, false, kSwedish, true), '[');
    EXPECT_EQ(typedCharacterFor(Key::Digit9, false, kSwedish, true), ']');
    EXPECT_EQ(typedCharacterFor(Key::Digit2, true, kSwedish), '"');
    EXPECT_EQ(typedCharacterFor(Key::Period, true, kSwedish), ':');
    EXPECT_EQ(typedCharacterFor(Key::Digit1, false, kSwedish, true), '\0');
}

TEST(TypingTest, TypedCharacterFor_HoldsAltAsideOnTheEnglishBoard)
{
    EXPECT_EQ(
        typedCharacterFor(Key::Digit7, false, KeyboardLayout::English, true),
        '7');
}

TEST(TypingTest, TypedCharacterFor_TypesAKeypadDigitAsItsDigit)
{
    EXPECT_EQ(
        typedCharacterFor(Key::Keypad0, false, KeyboardLayout::English),
        '0');
    EXPECT_EQ(
        typedCharacterFor(Key::Keypad9, true, KeyboardLayout::Swedish),
        '9');
}

TEST(TypingTest, TypedCharacterFor_TypesAnApostropheOnTheSwedishBoard)
{
    EXPECT_EQ(
        typedCharacterFor(Key::Backslash, false, KeyboardLayout::Swedish),
        '\'');
    EXPECT_EQ(
        typedCharacterFor(Key::Backslash, true, KeyboardLayout::Swedish),
        '*');
}
