#include <gtest/gtest.h>

#include <antwika/input/Key.hpp>

#include <antwika/sudoku/KeyMapping.hpp>

using antwika::input::Key;
using antwika::sudoku::digitFor;

TEST(KeyMappingTest, DigitFor_ReadsEveryDigitKeyAsItsOwnDigit)
{
    EXPECT_EQ(digitFor(Key::Digit0), 0);
    EXPECT_EQ(digitFor(Key::Digit1), 1);
    EXPECT_EQ(digitFor(Key::Digit5), 5);
    EXPECT_EQ(digitFor(Key::Digit9), 9);
}

TEST(KeyMappingTest, DigitFor_ReadsBackspaceAndDeleteAsEmptying)
{
    EXPECT_EQ(digitFor(Key::Backspace), 0);
    EXPECT_EQ(digitFor(Key::Delete), 0);
}

TEST(KeyMappingTest, DigitFor_AnswersNothingForAKeyWithNoMeaning)
{
    EXPECT_FALSE(digitFor(Key::A).has_value());
    EXPECT_FALSE(digitFor(Key::Z).has_value());
    EXPECT_FALSE(digitFor(Key::Escape).has_value());
    EXPECT_FALSE(digitFor(Key::Space).has_value());
}
