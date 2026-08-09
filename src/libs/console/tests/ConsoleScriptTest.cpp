#include <gtest/gtest.h>

#include <vector>

#include <antwika/console/testing/ConsoleScript.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/Key.hpp>

#include "antwika/console/KeyboardLayout.hpp"
#include "antwika/console/Typing.hpp"

using antwika::console::KeyboardLayout;
using antwika::console::typedCharacterFor;
using antwika::console::testing::keyAt;
using antwika::console::testing::kOpenTick;
using antwika::console::testing::pressThatTypes;
using antwika::console::testing::typeText;
using antwika::event::TickEvent;
using antwika::input::InputEventCodec;
using antwika::input::Key;
using antwika::input::KeyPressed;

TEST(ConsoleScriptTest, PressThatTypes_FindsTheKeyThatNeedsNoShift)
{
    const auto press = pressThatTypes('a', KeyboardLayout::English);

    EXPECT_EQ(press.key, Key::A);
    EXPECT_FALSE(press.modifiers.shift);
}

TEST(ConsoleScriptTest, PressThatTypes_FindsTheKeyThatNeedsShiftHeld)
{
    const auto press = pressThatTypes('_', KeyboardLayout::Swedish);

    EXPECT_EQ(press.key, Key::Slash);
    EXPECT_TRUE(press.modifiers.shift);
    EXPECT_EQ(
        typedCharacterFor(
            press.key, press.modifiers.shift, KeyboardLayout::Swedish),
        '_');
}

TEST(ConsoleScriptTest, PressThatTypes_AnswersADeadKeyForWhatNoBoardTypes)
{
    const auto press = pressThatTypes('\t', KeyboardLayout::Swedish);

    EXPECT_EQ(press.key, Key::CapsLock);
    EXPECT_FALSE(press.modifiers.shift);
}

TEST(ConsoleScriptTest, KOpenTick_IsTheFirstTickTheSheetIsFullyOut)
{
    EXPECT_EQ(kOpenTick, antwika::console::kConsoleAnimTicks + 1);
    EXPECT_EQ(kOpenTick, 9U);
}

TEST(ConsoleScriptTest, KeyAt_HoldsNoShiftUntilOneIsAskedFor)
{
    const InputEventCodec codec;

    const auto plain = keyAt(codec, 1, Key::A);
    const auto shifted = keyAt(codec, 1, Key::A, true);

    EXPECT_EQ(plain.tick, 1U);
    EXPECT_EQ(
        plain.event,
        codec.encode(
            KeyPressed{.key = Key::A, .modifiers = {.shift = false}}));
    EXPECT_EQ(
        shifted.event,
        codec.encode(
            KeyPressed{.key = Key::A, .modifiers = {.shift = true}}));
}

TEST(ConsoleScriptTest, TypeText_AppendsOnePressPerCharacter)
{
    const InputEventCodec codec;

    std::vector<TickEvent> events;
    typeText(events, codec, 2, "hi", KeyboardLayout::English);

    ASSERT_EQ(events.size(), 2U);
    EXPECT_EQ(events[0].tick, 2U);
    EXPECT_EQ(
        events[0].event,
        codec.encode(
            KeyPressed{.key = Key::H, .modifiers = {.shift = false}}));
    EXPECT_EQ(
        events[1].event,
        codec.encode(
            KeyPressed{.key = Key::I, .modifiers = {.shift = false}}));
}
