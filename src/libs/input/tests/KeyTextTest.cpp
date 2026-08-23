#include <gtest/gtest.h>

#include "antwika/input/KeyText.hpp"

using antwika::input::getCharTypedBy;
using antwika::input::Key;

TEST(KeyTextTest, CharTypedBy_TakesTheLettersBothWaysAbout)
{
    EXPECT_EQ(getCharTypedBy(Key::A, false), "a");
    EXPECT_EQ(getCharTypedBy(Key::Z, false), "z");
    EXPECT_EQ(getCharTypedBy(Key::A, true), "A");
    EXPECT_EQ(getCharTypedBy(Key::Z, true), "Z");
}

TEST(KeyTextTest, CharTypedBy_TakesTheDigitsAndTheMarksANameHolds)
{
    EXPECT_EQ(getCharTypedBy(Key::Digit0, false), "0");
    EXPECT_EQ(getCharTypedBy(Key::Digit9, false), "9");
    EXPECT_EQ(getCharTypedBy(Key::Minus, false), "-");
    EXPECT_EQ(getCharTypedBy(Key::Minus, true), "_");
    EXPECT_EQ(getCharTypedBy(Key::Period, false), ".");
    EXPECT_EQ(getCharTypedBy(Key::Space, false), " ");
}

TEST(KeyTextTest, CharTypedBy_TakesTheUpperMarkOfEveryDigit)
{
    EXPECT_EQ(getCharTypedBy(Key::Digit0, true), ")");
    EXPECT_EQ(getCharTypedBy(Key::Digit1, true), "!");
    EXPECT_EQ(getCharTypedBy(Key::Digit2, true), "@");
    EXPECT_EQ(getCharTypedBy(Key::Digit3, true), "#");
    EXPECT_EQ(getCharTypedBy(Key::Digit4, true), "$");
    EXPECT_EQ(getCharTypedBy(Key::Digit5, true), "%");
    EXPECT_EQ(getCharTypedBy(Key::Digit6, true), "^");
    EXPECT_EQ(getCharTypedBy(Key::Digit7, true), "&");
    EXPECT_EQ(getCharTypedBy(Key::Digit8, true), "*");
    EXPECT_EQ(getCharTypedBy(Key::Digit9, true), "(");
}

TEST(KeyTextTest, CharTypedBy_TakesEveryMarkOnItsOwn)
{
    EXPECT_EQ(getCharTypedBy(Key::Equal, false), "=");
    EXPECT_EQ(getCharTypedBy(Key::LeftBracket, false), "[");
    EXPECT_EQ(getCharTypedBy(Key::RightBracket, false), "]");
    EXPECT_EQ(getCharTypedBy(Key::Backslash, false), "\\");
    EXPECT_EQ(getCharTypedBy(Key::Semicolon, false), ";");
    EXPECT_EQ(getCharTypedBy(Key::Apostrophe, false), "'");
    EXPECT_EQ(getCharTypedBy(Key::Grave, false), "`");
    EXPECT_EQ(getCharTypedBy(Key::Comma, false), ",");
    EXPECT_EQ(getCharTypedBy(Key::Slash, false), "/");
}

TEST(KeyTextTest, CharTypedBy_TakesTheUpperMarkOfEveryMark)
{
    EXPECT_EQ(getCharTypedBy(Key::Equal, true), "+");
    EXPECT_EQ(getCharTypedBy(Key::LeftBracket, true), "{");
    EXPECT_EQ(getCharTypedBy(Key::RightBracket, true), "}");
    EXPECT_EQ(getCharTypedBy(Key::Backslash, true), "|");
    EXPECT_EQ(getCharTypedBy(Key::Semicolon, true), ":");
    EXPECT_EQ(getCharTypedBy(Key::Apostrophe, true), "\"");
    EXPECT_EQ(getCharTypedBy(Key::Grave, true), "~");
    EXPECT_EQ(getCharTypedBy(Key::Comma, true), "<");
    EXPECT_EQ(getCharTypedBy(Key::Period, true), ">");
    EXPECT_EQ(getCharTypedBy(Key::Slash, true), "?");
}

TEST(KeyTextTest, CharTypedBy_TakesASpaceHoweverShiftStands)
{
    EXPECT_EQ(getCharTypedBy(Key::Space, true), " ");
}

TEST(KeyTextTest, CharTypedBy_TakesNothingFromAKeyThatMarksNothing)
{
    EXPECT_TRUE(getCharTypedBy(Key::Enter, false).empty());
    EXPECT_TRUE(getCharTypedBy(Key::Escape, false).empty());
    EXPECT_TRUE(getCharTypedBy(Key::F5, false).empty());
    EXPECT_TRUE(getCharTypedBy(Key::Tab, true).empty());
}
