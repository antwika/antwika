#include <gtest/gtest.h>

#include <variant>

#include "antwika/input/InputEvent.hpp"
#include "antwika/input/Key.hpp"
#include "antwika/input/KeyModifiers.hpp"
#include "antwika/input/MouseButton.hpp"
#include "antwika/input/Position.hpp"

using antwika::input::InputEvent;
using antwika::input::Key;
using antwika::input::KeyModifiers;
using antwika::input::KeyPressed;
using antwika::input::KeyReleased;
using antwika::input::MouseButton;
using antwika::input::PointerButtonPressed;
using antwika::input::PointerButtonReleased;
using antwika::input::PointerMoved;
using antwika::input::PointerScrolled;
using antwika::input::Position;

TEST(InputEventTest, KeyPressed_DefaultsToAFreshPressWithNoModifiers)
{
    constexpr KeyPressed pressed;

    EXPECT_FALSE(pressed.repeat);
    EXPECT_EQ(pressed.modifiers, KeyModifiers{});
}

TEST(InputEventTest, KeyPressed_EqualityIsTrueForTheSamePress)
{
    constexpr KeyPressed one{
        .key = Key::W, .modifiers = {.shift = true}, .repeat = false};
    constexpr KeyPressed same{
        .key = Key::W, .modifiers = {.shift = true}, .repeat = false};

    EXPECT_EQ(one, same);
}

TEST(InputEventTest, KeyPressed_EqualityIsFalseWhenTheKeyDiffers)
{
    constexpr KeyPressed one{.key = Key::W};
    constexpr KeyPressed other{.key = Key::S};

    EXPECT_NE(one, other);
}

TEST(InputEventTest, KeyPressed_EqualityIsFalseWhenTheModifiersDiffer)
{
    constexpr KeyPressed one{.key = Key::W, .modifiers = {.shift = true}};
    constexpr KeyPressed other{.key = Key::W, .modifiers = {.shift = false}};

    EXPECT_NE(one, other);
}

TEST(InputEventTest, KeyPressed_EqualityIsFalseWhenTheRepeatFlagDiffers)
{
    // A repeat is a different event from a fresh press.
    // Anything reacting to a press rather than typing must tell them apart.
    constexpr KeyPressed one{.key = Key::W, .repeat = false};
    constexpr KeyPressed other{.key = Key::W, .repeat = true};

    EXPECT_NE(one, other);
}

TEST(InputEventTest, KeyReleased_EqualityComparesTheKeyAndTheModifiers)
{
    constexpr KeyReleased one{.key = Key::W, .modifiers = {.alt = true}};
    constexpr KeyReleased same{.key = Key::W, .modifiers = {.alt = true}};
    constexpr KeyReleased otherKey{.key = Key::S, .modifiers = {.alt = true}};
    constexpr KeyReleased otherMods{
        .key = Key::W, .modifiers = {.alt = false}};

    EXPECT_EQ(one, same);
    EXPECT_NE(one, otherKey);
    EXPECT_NE(one, otherMods);
}

TEST(InputEventTest, PointerMoved_EqualityComparesThePosition)
{
    constexpr PointerMoved one{.position = {.x = 4, .y = 8}};
    constexpr PointerMoved same{.position = {.x = 4, .y = 8}};
    constexpr PointerMoved other{.position = {.x = 4, .y = 9}};

    EXPECT_EQ(one, same);
    EXPECT_NE(one, other);
}

TEST(InputEventTest, PointerButtonPressed_EqualityComparesEveryField)
{
    constexpr PointerButtonPressed one{
        .button = MouseButton::Left,
        .position = {.x = 4, .y = 8},
        .modifiers = {.control = true}};

    constexpr PointerButtonPressed same{
        .button = MouseButton::Left,
        .position = {.x = 4, .y = 8},
        .modifiers = {.control = true}};

    constexpr PointerButtonPressed otherButton{
        .button = MouseButton::Right,
        .position = {.x = 4, .y = 8},
        .modifiers = {.control = true}};

    constexpr PointerButtonPressed otherPosition{
        .button = MouseButton::Left,
        .position = {.x = 5, .y = 8},
        .modifiers = {.control = true}};

    constexpr PointerButtonPressed otherModifiers{
        .button = MouseButton::Left,
        .position = {.x = 4, .y = 8},
        .modifiers = {.control = false}};

    EXPECT_EQ(one, same);
    EXPECT_NE(one, otherButton);
    EXPECT_NE(one, otherPosition);
    EXPECT_NE(one, otherModifiers);
}

TEST(InputEventTest, PointerButtonReleased_EqualityComparesEveryField)
{
    constexpr PointerButtonReleased one{
        .button = MouseButton::Middle,
        .position = {.x = 1, .y = 2},
        .modifiers = {.super = true}};

    constexpr PointerButtonReleased same{
        .button = MouseButton::Middle,
        .position = {.x = 1, .y = 2},
        .modifiers = {.super = true}};

    constexpr PointerButtonReleased otherButton{
        .button = MouseButton::X1,
        .position = {.x = 1, .y = 2},
        .modifiers = {.super = true}};

    constexpr PointerButtonReleased otherPosition{
        .button = MouseButton::Middle,
        .position = {.x = 1, .y = 3},
        .modifiers = {.super = true}};

    constexpr PointerButtonReleased otherModifiers{
        .button = MouseButton::Middle,
        .position = {.x = 1, .y = 2},
        .modifiers = {.super = false}};

    EXPECT_EQ(one, same);
    EXPECT_NE(one, otherButton);
    EXPECT_NE(one, otherPosition);
    EXPECT_NE(one, otherModifiers);
}

TEST(InputEventTest, PointerScrolled_DefaultsToNoMovementOnEitherAxis)
{
    constexpr PointerScrolled still;

    EXPECT_EQ(still.horizontal, 0);
    EXPECT_EQ(still.vertical, 0);
}

TEST(InputEventTest, PointerScrolled_EqualityComparesBothAxes)
{
    constexpr PointerScrolled one{.horizontal = 0, .vertical = 3};
    constexpr PointerScrolled same{.horizontal = 0, .vertical = 3};
    constexpr PointerScrolled otherVertical{.horizontal = 0, .vertical = -3};
    constexpr PointerScrolled otherHorizontal{
        .horizontal = 1, .vertical = 3};

    EXPECT_EQ(one, same);
    EXPECT_NE(one, otherVertical);
    EXPECT_NE(one, otherHorizontal);
}

TEST(InputEventTest, PointerScrolled_ReportsWholeNotchesInBothDirections)
{
    // Signed, so a scroll towards the user is not a second event kind.
    constexpr PointerScrolled down{.horizontal = -2, .vertical = -1};

    EXPECT_EQ(down.horizontal, -2);
    EXPECT_EQ(down.vertical, -1);
}

TEST(InputEventTest, InputEvent_HoldsEachAlternative)
{
    const InputEvent pressed = KeyPressed{.key = Key::Escape};
    const InputEvent released = KeyReleased{.key = Key::Escape};
    const InputEvent moved = PointerMoved{.position = {.x = 1, .y = 1}};
    const InputEvent buttonDown =
        PointerButtonPressed{.button = MouseButton::Left};
    const InputEvent buttonUp =
        PointerButtonReleased{.button = MouseButton::Left};
    const InputEvent scrolled = PointerScrolled{.vertical = 1};

    EXPECT_TRUE(std::holds_alternative<KeyPressed>(pressed));
    EXPECT_TRUE(std::holds_alternative<KeyReleased>(released));
    EXPECT_TRUE(std::holds_alternative<PointerMoved>(moved));
    EXPECT_TRUE(std::holds_alternative<PointerButtonPressed>(buttonDown));
    EXPECT_TRUE(std::holds_alternative<PointerButtonReleased>(buttonUp));
    EXPECT_TRUE(std::holds_alternative<PointerScrolled>(scrolled));
}

TEST(InputEventTest, InputEvent_EqualityDistinguishesTheAlternatives)
{
    // A press and a release of one key differ, though their fields match.
    const InputEvent pressed = KeyPressed{.key = Key::Escape};
    const InputEvent released = KeyReleased{.key = Key::Escape};

    EXPECT_NE(pressed, released);
}

TEST(InputEventTest, InputEvent_DefaultsToAKeyPress)
{
    // Only because KeyPressed is first in the variant.
    // Nothing should rely on that, but a default is not "no event".
    const InputEvent event;

    EXPECT_TRUE(std::holds_alternative<KeyPressed>(event));
}
