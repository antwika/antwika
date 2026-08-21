#include <gtest/gtest.h>

#include "antwika/input/KeyText.hpp"

using antwika::input::charTypedBy;
using antwika::input::Key;

TEST(KeyTextTest, CharTypedBy_TakesTheLettersBothWaysAbout)
{
    EXPECT_EQ(charTypedBy(Key::A, false), "a");
    EXPECT_EQ(charTypedBy(Key::Z, false), "z");
    EXPECT_EQ(charTypedBy(Key::A, true), "A");
    EXPECT_EQ(charTypedBy(Key::Z, true), "Z");
}

TEST(KeyTextTest, CharTypedBy_TakesTheDigitsAndTheMarksANameHolds)
{
    EXPECT_EQ(charTypedBy(Key::Digit0, false), "0");
    EXPECT_EQ(charTypedBy(Key::Digit9, false), "9");
    EXPECT_EQ(charTypedBy(Key::Minus, false), "-");
    EXPECT_EQ(charTypedBy(Key::Minus, true), "_");
    EXPECT_EQ(charTypedBy(Key::Period, false), ".");
    EXPECT_EQ(charTypedBy(Key::Space, false), " ");
}

TEST(KeyTextTest, CharTypedBy_TakesTheUpperMarkOfEveryDigit)
{
    EXPECT_EQ(charTypedBy(Key::Digit0, true), ")");
    EXPECT_EQ(charTypedBy(Key::Digit1, true), "!");
    EXPECT_EQ(charTypedBy(Key::Digit2, true), "@");
    EXPECT_EQ(charTypedBy(Key::Digit3, true), "#");
    EXPECT_EQ(charTypedBy(Key::Digit4, true), "$");
    EXPECT_EQ(charTypedBy(Key::Digit5, true), "%");
    EXPECT_EQ(charTypedBy(Key::Digit6, true), "^");
    EXPECT_EQ(charTypedBy(Key::Digit7, true), "&");
    EXPECT_EQ(charTypedBy(Key::Digit8, true), "*");
    EXPECT_EQ(charTypedBy(Key::Digit9, true), "(");
}

TEST(KeyTextTest, CharTypedBy_TakesEveryMarkOnItsOwn)
{
    EXPECT_EQ(charTypedBy(Key::Equal, false), "=");
    EXPECT_EQ(charTypedBy(Key::LeftBracket, false), "[");
    EXPECT_EQ(charTypedBy(Key::RightBracket, false), "]");
    EXPECT_EQ(charTypedBy(Key::Backslash, false), "\\");
    EXPECT_EQ(charTypedBy(Key::Semicolon, false), ";");
    EXPECT_EQ(charTypedBy(Key::Apostrophe, false), "'");
    EXPECT_EQ(charTypedBy(Key::Grave, false), "`");
    EXPECT_EQ(charTypedBy(Key::Comma, false), ",");
    EXPECT_EQ(charTypedBy(Key::Slash, false), "/");
}

TEST(KeyTextTest, CharTypedBy_TakesTheUpperMarkOfEveryMark)
{
    EXPECT_EQ(charTypedBy(Key::Equal, true), "+");
    EXPECT_EQ(charTypedBy(Key::LeftBracket, true), "{");
    EXPECT_EQ(charTypedBy(Key::RightBracket, true), "}");
    EXPECT_EQ(charTypedBy(Key::Backslash, true), "|");
    EXPECT_EQ(charTypedBy(Key::Semicolon, true), ":");
    EXPECT_EQ(charTypedBy(Key::Apostrophe, true), "\"");
    EXPECT_EQ(charTypedBy(Key::Grave, true), "~");
    EXPECT_EQ(charTypedBy(Key::Comma, true), "<");
    EXPECT_EQ(charTypedBy(Key::Period, true), ">");
    EXPECT_EQ(charTypedBy(Key::Slash, true), "?");
}

TEST(KeyTextTest, CharTypedBy_TakesASpaceHoweverShiftStands)
{
    EXPECT_EQ(charTypedBy(Key::Space, true), " ");
}

TEST(KeyTextTest, CharTypedBy_TakesNothingFromAKeyThatMarksNothing)
{
    EXPECT_TRUE(charTypedBy(Key::Enter, false).empty());
    EXPECT_TRUE(charTypedBy(Key::Escape, false).empty());
    EXPECT_TRUE(charTypedBy(Key::F5, false).empty());
    EXPECT_TRUE(charTypedBy(Key::Tab, true).empty());
}
