#include <gtest/gtest.h>

#include <antwika/input/Key.hpp>
#include <antwika/ui/Keyboard.hpp>

#include "antwika/map_editor/UiKeyMapping.hpp"

using antwika::input::Key;
using antwika::map_editor::typedCharacterFor;
using antwika::map_editor::uiKeyFor;

TEST(UiKeyMappingTest, UiKeyFor_MovesFocusWithTabAndShiftTab)
{
    EXPECT_EQ(uiKeyFor(Key::Tab, false), antwika::ui::Key::FocusNext);
    EXPECT_EQ(uiKeyFor(Key::Tab, true), antwika::ui::Key::FocusPrevious);
}

TEST(UiKeyMappingTest, UiKeyFor_MapsTheEditingKeys)
{
    EXPECT_EQ(uiKeyFor(Key::Enter, false), antwika::ui::Key::Activate);
    EXPECT_EQ(
        uiKeyFor(Key::Backspace, false), antwika::ui::Key::Backspace);
    EXPECT_EQ(uiKeyFor(Key::Delete, false), antwika::ui::Key::Delete);
    EXPECT_EQ(uiKeyFor(Key::Escape, false), antwika::ui::Key::Cancel);
}

TEST(UiKeyMappingTest, UiKeyFor_IgnoresShiftOnTheEditingKeys)
{
    EXPECT_EQ(uiKeyFor(Key::Enter, true), antwika::ui::Key::Activate);
    EXPECT_EQ(
        uiKeyFor(Key::Backspace, true), antwika::ui::Key::Backspace);
    EXPECT_EQ(uiKeyFor(Key::Delete, true), antwika::ui::Key::Delete);
    EXPECT_EQ(uiKeyFor(Key::Escape, true), antwika::ui::Key::Cancel);
}

TEST(UiKeyMappingTest, UiKeyFor_SelectsWithShiftedMovement)
{
    EXPECT_EQ(
        uiKeyFor(Key::ArrowLeft, false), antwika::ui::Key::MoveLeft);
    EXPECT_EQ(
        uiKeyFor(Key::ArrowLeft, true), antwika::ui::Key::SelectLeft);
    EXPECT_EQ(
        uiKeyFor(Key::ArrowRight, false), antwika::ui::Key::MoveRight);
    EXPECT_EQ(
        uiKeyFor(Key::ArrowRight, true), antwika::ui::Key::SelectRight);
    EXPECT_EQ(
        uiKeyFor(Key::Home, false), antwika::ui::Key::MoveLineStart);
    EXPECT_EQ(
        uiKeyFor(Key::Home, true), antwika::ui::Key::SelectLineStart);
    EXPECT_EQ(uiKeyFor(Key::End, false), antwika::ui::Key::MoveLineEnd);
    EXPECT_EQ(
        uiKeyFor(Key::End, true), antwika::ui::Key::SelectLineEnd);
}

TEST(UiKeyMappingTest, UiKeyFor_YieldsNothingForAKeyItDoesNotMap)
{
    EXPECT_FALSE(uiKeyFor(Key::A, false).has_value());
    EXPECT_FALSE(uiKeyFor(Key::F1, true).has_value());
}

TEST(UiKeyMappingTest, TypedCharacterFor_TypesTheUnshiftedDigits)
{
    EXPECT_EQ(typedCharacterFor(Key::Digit0, false), '0');
    EXPECT_EQ(typedCharacterFor(Key::Digit9, false), '9');
}

TEST(UiKeyMappingTest, TypedCharacterFor_SwallowsAShiftedDigit)
{
    EXPECT_EQ(typedCharacterFor(Key::Digit0, true), '\0');
}

TEST(UiKeyMappingTest, TypedCharacterFor_CasesLettersByShift)
{
    EXPECT_EQ(typedCharacterFor(Key::A, false), 'a');
    EXPECT_EQ(typedCharacterFor(Key::Z, false), 'z');
    EXPECT_EQ(typedCharacterFor(Key::A, true), 'A');
    EXPECT_EQ(typedCharacterFor(Key::Z, true), 'Z');
}

TEST(UiKeyMappingTest, TypedCharacterFor_TypesSpaceEitherWay)
{
    EXPECT_EQ(typedCharacterFor(Key::Space, false), ' ');
    EXPECT_EQ(typedCharacterFor(Key::Space, true), ' ');
}

TEST(UiKeyMappingTest, TypedCharacterFor_TypesUnderscoreForShiftedMinus)
{
    EXPECT_EQ(typedCharacterFor(Key::Minus, false), '-');
    EXPECT_EQ(typedCharacterFor(Key::Minus, true), '_');
}

TEST(UiKeyMappingTest, TypedCharacterFor_SwallowsShiftedPunctuation)
{
    EXPECT_EQ(typedCharacterFor(Key::Period, false), '.');
    EXPECT_EQ(typedCharacterFor(Key::Period, true), '\0');
    EXPECT_EQ(typedCharacterFor(Key::Comma, false), ',');
    EXPECT_EQ(typedCharacterFor(Key::Comma, true), '\0');
}

TEST(UiKeyMappingTest, TypedCharacterFor_TypesNothingForAControlKey)
{
    EXPECT_EQ(typedCharacterFor(Key::Tab, false), '\0');
    EXPECT_EQ(typedCharacterFor(Key::F5, true), '\0');
}
