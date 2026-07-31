#include <gtest/gtest.h>

#include <antwika/input/Key.hpp>
#include <antwika/ui/Keyboard.hpp>

#include "antwika/game/KeyText.hpp"

namespace
{

    using antwika::game::typedCharacterFor;
    using antwika::game::uiKeyFor;
    using antwika::input::Key;
    using UiKey = antwika::ui::Key;

    TEST(KeyTextTest, UiKeyFor_TellsTabFromShiftTab)
    {
        EXPECT_EQ(uiKeyFor(Key::Tab, false), UiKey::FocusNext);
        EXPECT_EQ(uiKeyFor(Key::Tab, true), UiKey::FocusPrevious);
    }

    TEST(KeyTextTest, UiKeyFor_NamesTheKeysTheUiActsOn)
    {
        EXPECT_EQ(uiKeyFor(Key::Enter, false), UiKey::Activate);
        EXPECT_EQ(uiKeyFor(Key::Backspace, false), UiKey::Backspace);
        EXPECT_EQ(uiKeyFor(Key::ArrowLeft, false), UiKey::MoveLeft);
        EXPECT_EQ(uiKeyFor(Key::ArrowRight, false), UiKey::MoveRight);
    }

    // Escape ends a run here, upstream of every sink.
    // So it is deliberately not the UI's Cancel.
    TEST(KeyTextTest, UiKeyFor_HasNoMeaningForAnythingElse)
    {
        EXPECT_FALSE(uiKeyFor(Key::Escape, false).has_value());
        EXPECT_FALSE(uiKeyFor(Key::A, false).has_value());
        EXPECT_FALSE(uiKeyFor(Key::F1, true).has_value());
    }

    TEST(KeyTextTest, TypedCharacterFor_TypesLettersInEitherCase)
    {
        EXPECT_EQ(typedCharacterFor(Key::A, false), 'a');
        EXPECT_EQ(typedCharacterFor(Key::Z, false), 'z');
        EXPECT_EQ(typedCharacterFor(Key::A, true), 'A');
        EXPECT_EQ(typedCharacterFor(Key::Z, true), 'Z');
    }

    TEST(KeyTextTest, TypedCharacterFor_TypesUnshiftedDigits)
    {
        EXPECT_EQ(typedCharacterFor(Key::Digit0, false), '0');
        EXPECT_EQ(typedCharacterFor(Key::Digit9, false), '9');
        EXPECT_EQ(typedCharacterFor(Key::Digit1, true), '\0');
    }

    TEST(KeyTextTest, TypedCharacterFor_TypesWhatAFileNameNeeds)
    {
        EXPECT_EQ(typedCharacterFor(Key::Space, false), ' ');
        EXPECT_EQ(typedCharacterFor(Key::Minus, false), '-');
        EXPECT_EQ(typedCharacterFor(Key::Minus, true), '_');
        EXPECT_EQ(typedCharacterFor(Key::Period, false), '.');
        EXPECT_EQ(typedCharacterFor(Key::Period, true), '\0');
    }

    TEST(KeyTextTest, TypedCharacterFor_TypesNothingForEverythingElse)
    {
        EXPECT_EQ(typedCharacterFor(Key::Tab, false), '\0');
        EXPECT_EQ(typedCharacterFor(Key::Escape, true), '\0');
        EXPECT_EQ(typedCharacterFor(Key::F12, false), '\0');
    }

} // namespace
