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
    constexpr KeyPressed pressedEvent;

    EXPECT_FALSE(pressedEvent.repeat);
    EXPECT_EQ(pressedEvent.modifiers, KeyModifiers{});
}

TEST(InputEventTest, KeyPressed_EqualityIsTrueForTheSamePress)
{
    constexpr KeyPressed onePressed{
        .key = Key::W, .modifiers = {.shift = true}, .repeat = false};
    constexpr KeyPressed samePressed{
        .key = Key::W, .modifiers = {.shift = true}, .repeat = false};

    EXPECT_EQ(onePressed, samePressed);
}

TEST(InputEventTest, KeyPressed_EqualityIsFalseWhenTheKeyDiffers)
{
    constexpr KeyPressed onePressed{.key = Key::W};
    constexpr KeyPressed otherPressed{.key = Key::S};

    EXPECT_NE(onePressed, otherPressed);
}

TEST(InputEventTest, KeyPressed_EqualityIsFalseWhenTheModifiersDiffer)
{
    constexpr KeyPressed onePressed{
        .key = Key::W,
        .modifiers = {.shift = true}};
    constexpr KeyPressed otherPressed{
        .key = Key::W,
        .modifiers = {.shift = false}};

    EXPECT_NE(onePressed, otherPressed);
}

TEST(InputEventTest, KeyPressed_EqualityIsFalseWhenTheRepeatFlagDiffers)
{
    constexpr KeyPressed onePressed{.key = Key::W, .repeat = false};
    constexpr KeyPressed otherPressed{.key = Key::W, .repeat = true};

    EXPECT_NE(onePressed, otherPressed);
}

TEST(InputEventTest, KeyReleased_EqualityComparesTheKeyAndTheModifiers)
{
    constexpr KeyReleased oneReleased{
        .key = Key::W,
        .modifiers = {.alt = true}};
    constexpr KeyReleased sameReleased{
        .key = Key::W,
        .modifiers = {.alt = true}};
    constexpr KeyReleased otherKey{.key = Key::S, .modifiers = {.alt = true}};
    constexpr KeyReleased otherModsReleased{
        .key = Key::W, .modifiers = {.alt = false}};

    EXPECT_EQ(oneReleased, sameReleased);
    EXPECT_NE(oneReleased, otherKey);
    EXPECT_NE(oneReleased, otherModsReleased);
}

TEST(InputEventTest, PointerMoved_EqualityComparesThePosition)
{
    constexpr PointerMoved oneMoved{.position = {.x = 4, .y = 8}};
    constexpr PointerMoved sameMoved{.position = {.x = 4, .y = 8}};
    constexpr PointerMoved otherMoved{.position = {.x = 4, .y = 9}};

    EXPECT_EQ(oneMoved, sameMoved);
    EXPECT_NE(oneMoved, otherMoved);
}

TEST(InputEventTest, PointerButtonPressed_EqualityComparesEveryField)
{
    constexpr PointerButtonPressed onePressed{
        .button = MouseButton::Left,
        .position = {.x = 4, .y = 8},
        .modifiers = {.control = true}};

    constexpr PointerButtonPressed samePressed{
        .button = MouseButton::Left,
        .position = {.x = 4, .y = 8},
        .modifiers = {.control = true}};

    constexpr PointerButtonPressed otherButton{
        .button = MouseButton::Right,
        .position = {.x = 4, .y = 8},
        .modifiers = {.control = true}};

    constexpr PointerButtonPressed otherPositionPressed{
        .button = MouseButton::Left,
        .position = {.x = 5, .y = 8},
        .modifiers = {.control = true}};

    constexpr PointerButtonPressed otherModifiersPressed{
        .button = MouseButton::Left,
        .position = {.x = 4, .y = 8},
        .modifiers = {.control = false}};

    EXPECT_EQ(onePressed, samePressed);
    EXPECT_NE(onePressed, otherButton);
    EXPECT_NE(onePressed, otherPositionPressed);
    EXPECT_NE(onePressed, otherModifiersPressed);
}

TEST(InputEventTest, PointerButtonReleased_EqualityComparesEveryField)
{
    constexpr PointerButtonReleased oneReleased{
        .button = MouseButton::Middle,
        .position = {.x = 1, .y = 2},
        .modifiers = {.super = true}};

    constexpr PointerButtonReleased sameReleased{
        .button = MouseButton::Middle,
        .position = {.x = 1, .y = 2},
        .modifiers = {.super = true}};

    constexpr PointerButtonReleased otherButton{
        .button = MouseButton::X1,
        .position = {.x = 1, .y = 2},
        .modifiers = {.super = true}};

    constexpr PointerButtonReleased otherPositionReleased{
        .button = MouseButton::Middle,
        .position = {.x = 1, .y = 3},
        .modifiers = {.super = true}};

    constexpr PointerButtonReleased otherModifiersReleased{
        .button = MouseButton::Middle,
        .position = {.x = 1, .y = 2},
        .modifiers = {.super = false}};

    EXPECT_EQ(oneReleased, sameReleased);
    EXPECT_NE(oneReleased, otherButton);
    EXPECT_NE(oneReleased, otherPositionReleased);
    EXPECT_NE(oneReleased, otherModifiersReleased);
}

TEST(InputEventTest, PointerScrolled_DefaultsToNoMovementOnEitherAxis)
{
    constexpr PointerScrolled stillScrolled;

    EXPECT_EQ(stillScrolled.horizontal, 0);
    EXPECT_EQ(stillScrolled.vertical, 0);
}

TEST(InputEventTest, PointerScrolled_EqualityComparesBothAxes)
{
    constexpr PointerScrolled oneScrolled{.horizontal = 0, .vertical = 3};
    constexpr PointerScrolled sameScrolled{.horizontal = 0, .vertical = 3};
    constexpr PointerScrolled otherVerticalScrolled{
        .horizontal = 0,
        .vertical = -3};
    constexpr PointerScrolled otherHorizontalScrolled{
        .horizontal = 1, .vertical = 3};

    EXPECT_EQ(oneScrolled, sameScrolled);
    EXPECT_NE(oneScrolled, otherVerticalScrolled);
    EXPECT_NE(oneScrolled, otherHorizontalScrolled);
}

TEST(InputEventTest, PointerScrolled_ReportsWholeNotchesInBothDirections)
{
    constexpr PointerScrolled downScrolled{.horizontal = -2, .vertical = -1};

    EXPECT_EQ(downScrolled.horizontal, -2);
    EXPECT_EQ(downScrolled.vertical, -1);
}

TEST(InputEventTest, InputEvent_HoldsEachAlternative)
{
    const InputEvent pressedEvent = KeyPressed{.key = Key::Escape};
    const InputEvent releasedEvent = KeyReleased{.key = Key::Escape};
    const InputEvent movedEvent = PointerMoved{.position = {.x = 1, .y = 1}};
    const InputEvent buttonDownEvent =
        PointerButtonPressed{.button = MouseButton::Left};
    const InputEvent buttonUpEvent =
        PointerButtonReleased{.button = MouseButton::Left};
    const InputEvent scrolledEvent = PointerScrolled{.vertical = 1};

    EXPECT_TRUE(std::holds_alternative<KeyPressed>(pressedEvent));
    EXPECT_TRUE(std::holds_alternative<KeyReleased>(releasedEvent));
    EXPECT_TRUE(std::holds_alternative<PointerMoved>(movedEvent));
    EXPECT_TRUE(std::holds_alternative<PointerButtonPressed>(buttonDownEvent));
    EXPECT_TRUE(std::holds_alternative<PointerButtonReleased>(buttonUpEvent));
    EXPECT_TRUE(std::holds_alternative<PointerScrolled>(scrolledEvent));
}

TEST(InputEventTest, InputEvent_EqualityDistinguishesTheAlternatives)
{
    const InputEvent pressedEvent = KeyPressed{.key = Key::Escape};
    const InputEvent releasedEvent = KeyReleased{.key = Key::Escape};

    EXPECT_NE(pressedEvent, releasedEvent);
}

TEST(InputEventTest, InputEvent_DefaultsToAKeyPress)
{
    const InputEvent event;

    EXPECT_TRUE(std::holds_alternative<KeyPressed>(event));
}
