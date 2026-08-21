#include <gtest/gtest.h>

#include "antwika/input/InputEvent.hpp"
#include "antwika/input/InputState.hpp"
#include "antwika/input/Key.hpp"
#include "antwika/input/MouseButton.hpp"
#include "antwika/input/Offset.hpp"
#include "antwika/input/Position.hpp"

using antwika::input::InputState;
using antwika::input::Key;
using antwika::input::KeyPressed;
using antwika::input::KeyReleased;
using antwika::input::MouseButton;
using antwika::input::Offset;
using antwika::input::PointerButtonPressed;
using antwika::input::PointerButtonReleased;
using antwika::input::PointerMoved;
using antwika::input::PointerScrolled;
using antwika::input::Position;

TEST(InputStateTest, Apply_RoutesAKeyPressToTheKeyboard)
{
    InputState state;

    state.apply(KeyPressed{.key = Key::W});

    EXPECT_TRUE(state.keyboard().isDown(Key::W));
    EXPECT_TRUE(state.keyboard().wasPressed(Key::W));
}

TEST(InputStateTest, Apply_RoutesAKeyReleaseToTheKeyboard)
{
    InputState state;
    state.apply(KeyPressed{.key = Key::W});

    state.apply(KeyReleased{.key = Key::W});

    EXPECT_FALSE(state.keyboard().isDown(Key::W));
    EXPECT_TRUE(state.keyboard().wasReleased(Key::W));
}

TEST(InputStateTest, Apply_RoutesAMovementToTheMouse)
{
    InputState state;

    state.apply(PointerMoved{.position = {.x = 20, .y = 30}});

    EXPECT_EQ(state.mouse().position(), (Position{.x = 20, .y = 30}));
}

TEST(InputStateTest, Apply_RoutesAButtonPressToTheMouse)
{
    InputState state;

    state.apply(PointerButtonPressed{.button = MouseButton::Left});

    EXPECT_TRUE(state.mouse().isDown(MouseButton::Left));
    EXPECT_TRUE(state.mouse().wasPressed(MouseButton::Left));
}

TEST(InputStateTest, Apply_RoutesAButtonReleaseToTheMouse)
{
    InputState state;
    state.apply(PointerButtonPressed{.button = MouseButton::Left});

    state.apply(PointerButtonReleased{.button = MouseButton::Left});

    EXPECT_TRUE(state.mouse().wasReleased(MouseButton::Left));
}

TEST(InputStateTest, Apply_RoutesAScrollToTheMouse)
{
    InputState state;

    state.apply(PointerScrolled{.vertical = 2});

    EXPECT_EQ(state.mouse().scroll(), (Offset{.x = 0, .y = 2}));
}

TEST(InputStateTest, Apply_FoldsAButtonPressesModifiersIntoTheKeyboard)
{
    InputState state;

    state.apply(
        PointerButtonPressed{
            .button = MouseButton::Left, .modifiers = {.shift = true}});

    EXPECT_TRUE(state.keyboard().modifiers().shift);
}

TEST(InputStateTest, Apply_FoldsAButtonReleasesModifiersIntoTheKeyboard)
{
    InputState state;

    state.apply(
        PointerButtonReleased{
            .button = MouseButton::Left, .modifiers = {.control = true}});

    EXPECT_TRUE(state.keyboard().modifiers().control);
}

TEST(InputStateTest, BeginTick_ClearsBothDevicesEdges)
{
    InputState state;
    state.apply(KeyPressed{.key = Key::W});
    state.apply(PointerButtonPressed{.button = MouseButton::Left});
    state.apply(PointerScrolled{.vertical = 1});

    state.beginTick();

    EXPECT_FALSE(state.keyboard().wasPressed(Key::W));
    EXPECT_FALSE(state.mouse().wasPressed(MouseButton::Left));
    EXPECT_EQ(state.mouse().scroll(), Offset{});
}

TEST(InputStateTest, BeginTick_KeepsWhatBothDevicesStillHold)
{
    InputState state;
    state.apply(KeyPressed{.key = Key::W});
    state.apply(PointerButtonPressed{.button = MouseButton::Left});

    state.beginTick();

    EXPECT_TRUE(state.keyboard().isDown(Key::W));
    EXPECT_TRUE(state.mouse().isDown(MouseButton::Left));
}
