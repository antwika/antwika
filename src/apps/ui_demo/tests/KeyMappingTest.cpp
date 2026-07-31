#include <gtest/gtest.h>

#include <antwika/input/Key.hpp>
#include <antwika/ui/Keyboard.hpp>

#include "antwika/ui_demo/KeyMapping.hpp"

using antwika::input::Key;
using antwika::ui_demo::typedCharacterFor;
using antwika::ui_demo::uiKeyFor;
using UiKey = antwika::ui::Key;

namespace
{
    TEST(KeyMappingTest, UiKeyFor_TellsTabFromShiftTab)
    {
        EXPECT_EQ(uiKeyFor(Key::Tab, false), UiKey::FocusNext);
        EXPECT_EQ(uiKeyFor(Key::Tab, true), UiKey::FocusPrevious);
    }

    TEST(KeyMappingTest, UiKeyFor_NamesTheKeysTheLibraryActsOn)
    {
        EXPECT_EQ(uiKeyFor(Key::Enter, false), UiKey::Activate);
        EXPECT_EQ(uiKeyFor(Key::Backspace, false), UiKey::Backspace);
        EXPECT_EQ(uiKeyFor(Key::Escape, false), UiKey::Cancel);
        EXPECT_EQ(uiKeyFor(Key::ArrowLeft, false), UiKey::MoveLeft);
        EXPECT_EQ(uiKeyFor(Key::ArrowRight, false), UiKey::MoveRight);
    }

    TEST(KeyMappingTest, UiKeyFor_SaysNothingForAKeyItHasNoMeaningFor)
    {
        EXPECT_FALSE(uiKeyFor(Key::F1, false).has_value());
        EXPECT_FALSE(uiKeyFor(Key::A, false).has_value());
    }

    TEST(KeyMappingTest, TypedCharacterFor_TypesLettersInEitherCase)
    {
        EXPECT_EQ(typedCharacterFor(Key::A, false), 'a');
        EXPECT_EQ(typedCharacterFor(Key::Z, false), 'z');
        EXPECT_EQ(typedCharacterFor(Key::A, true), 'A');
        EXPECT_EQ(typedCharacterFor(Key::Z, true), 'Z');
    }

    TEST(KeyMappingTest, TypedCharacterFor_TypesUnshiftedDigitsOnly)
    {
        EXPECT_EQ(typedCharacterFor(Key::Digit0, false), '0');
        EXPECT_EQ(typedCharacterFor(Key::Digit9, false), '9');
        EXPECT_EQ(typedCharacterFor(Key::Digit4, true), '\0');
    }

    TEST(KeyMappingTest, TypedCharacterFor_TypesTheFewMarksAFieldNeeds)
    {
        EXPECT_EQ(typedCharacterFor(Key::Space, false), ' ');
        EXPECT_EQ(typedCharacterFor(Key::Minus, false), '-');
        EXPECT_EQ(typedCharacterFor(Key::Minus, true), '_');
        EXPECT_EQ(typedCharacterFor(Key::Period, false), '.');
        EXPECT_EQ(typedCharacterFor(Key::Period, true), '\0');
    }

    TEST(KeyMappingTest, TypedCharacterFor_TypesNothingForEverythingElse)
    {
        EXPECT_EQ(typedCharacterFor(Key::F1, false), '\0');
        EXPECT_EQ(typedCharacterFor(Key::Tab, false), '\0');
    }
} // namespace
