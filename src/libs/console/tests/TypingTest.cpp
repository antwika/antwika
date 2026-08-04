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

// Enter is deliberately absent, since executing is the bound key.
// And Tab is absent because one field has no ring to walk.
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
