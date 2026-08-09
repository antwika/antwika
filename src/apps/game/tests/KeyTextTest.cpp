#include <gtest/gtest.h>

#include <antwika/input/Key.hpp>
#include <antwika/ui/Keyboard.hpp>

#include "antwika/game/KeyText.hpp"
#include "antwika/game/KeyboardLayout.hpp"

namespace
{

    using antwika::game::KeyboardLayout;
    using antwika::game::typedCharacterFor;
    using antwika::game::uiKeyFor;
    using antwika::input::Key;
    using UiKey = antwika::ui::Key;

    constexpr auto kEnglish = KeyboardLayout::English;
    constexpr auto kSwedish = KeyboardLayout::Swedish;

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

    TEST(KeyTextTest, UiKeyFor_HasNoMeaningForAnythingElse)
    {
        EXPECT_FALSE(uiKeyFor(Key::Escape, false).has_value());
        EXPECT_FALSE(uiKeyFor(Key::A, false).has_value());
        EXPECT_FALSE(uiKeyFor(Key::F1, true).has_value());
    }

    TEST(KeyTextTest, TypedCharacterFor_TypesLettersOnEitherBoard)
    {
        EXPECT_EQ(typedCharacterFor(Key::A, false, kEnglish), 'a');
        EXPECT_EQ(typedCharacterFor(Key::Z, false, kEnglish), 'z');
        EXPECT_EQ(typedCharacterFor(Key::A, true, kSwedish), 'A');
        EXPECT_EQ(typedCharacterFor(Key::Z, true, kSwedish), 'Z');
    }

    TEST(KeyTextTest, TypedCharacterFor_TypesUnshiftedDigitsOnEither)
    {
        EXPECT_EQ(typedCharacterFor(Key::Digit0, false, kEnglish), '0');
        EXPECT_EQ(typedCharacterFor(Key::Digit9, false, kSwedish), '9');

        EXPECT_EQ(typedCharacterFor(Key::Digit1, true, kEnglish), '\0');
    }

    TEST(KeyTextTest, TypedCharacterFor_TypesTheAmericanPunctuation)
    {
        EXPECT_EQ(typedCharacterFor(Key::Space, false, kEnglish), ' ');
        EXPECT_EQ(typedCharacterFor(Key::Minus, false, kEnglish), '-');
        EXPECT_EQ(typedCharacterFor(Key::Minus, true, kEnglish), '_');
        EXPECT_EQ(typedCharacterFor(Key::Period, false, kEnglish), '.');
        EXPECT_EQ(
            typedCharacterFor(Key::Period, true, kEnglish), '\0');
        EXPECT_EQ(
            typedCharacterFor(Key::Slash, false, kEnglish), '\0');
    }

    TEST(KeyTextTest, TypedCharacterFor_TypesWhatASwedishBoardPrints)
    {
        EXPECT_EQ(typedCharacterFor(Key::Minus, false, kSwedish), '+');
        EXPECT_EQ(typedCharacterFor(Key::Minus, true, kSwedish), '?');
        EXPECT_EQ(typedCharacterFor(Key::Slash, false, kSwedish), '-');
        EXPECT_EQ(typedCharacterFor(Key::Slash, true, kSwedish), '_');
        EXPECT_EQ(typedCharacterFor(Key::Period, false, kSwedish), '.');
        EXPECT_EQ(typedCharacterFor(Key::Period, true, kSwedish), ':');
        EXPECT_EQ(typedCharacterFor(Key::Comma, false, kSwedish), ',');
        EXPECT_EQ(typedCharacterFor(Key::Comma, true, kSwedish), ';');
    }

    TEST(KeyTextTest, TypedCharacterFor_TypesTheSwedishShiftedDigits)
    {
        EXPECT_EQ(typedCharacterFor(Key::Digit1, true, kSwedish), '!');
        EXPECT_EQ(typedCharacterFor(Key::Digit2, true, kSwedish), '"');
        EXPECT_EQ(typedCharacterFor(Key::Digit7, true, kSwedish), '/');
        EXPECT_EQ(typedCharacterFor(Key::Digit0, true, kSwedish), '=');

        EXPECT_EQ(
            typedCharacterFor(Key::Digit4, true, kSwedish), '\0');
    }

    TEST(KeyTextTest, TypedCharacterFor_TypesNothingForEverythingElse)
    {
        EXPECT_EQ(typedCharacterFor(Key::Tab, false, kEnglish), '\0');
        EXPECT_EQ(typedCharacterFor(Key::Escape, true, kSwedish), '\0');
        EXPECT_EQ(typedCharacterFor(Key::F12, false, kEnglish), '\0');
        EXPECT_EQ(typedCharacterFor(Key::Equal, false, kSwedish), '\0');
    }

}
