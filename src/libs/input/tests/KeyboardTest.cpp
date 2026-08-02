#include <gtest/gtest.h>

#include "antwika/input/Key.hpp"
#include "antwika/input/KeyModifiers.hpp"
#include "antwika/input/Keyboard.hpp"

using antwika::input::Key;
using antwika::input::Keyboard;
using antwika::input::KeyModifiers;
using antwika::input::KeyPressed;
using antwika::input::KeyReleased;

namespace
{
    // Past the last enumerator, so every guard has something to reject.
    constexpr auto kUnnamedKey = static_cast<Key>(200);
} // namespace

TEST(KeyboardTest, IsDown_ReportsNothingHeldBeforeAnyEvent)
{
    const Keyboard keyboard;

    EXPECT_FALSE(keyboard.isDown(Key::A));
    EXPECT_FALSE(keyboard.wasPressed(Key::A));
    EXPECT_FALSE(keyboard.wasReleased(Key::A));
    EXPECT_EQ(keyboard.modifiers(), KeyModifiers{});
}

TEST(KeyboardTest, Apply_MarksAPressedKeyHeldAndPressed)
{
    Keyboard keyboard;

    keyboard.apply(KeyPressed{.key = Key::W});

    EXPECT_TRUE(keyboard.isDown(Key::W));
    EXPECT_TRUE(keyboard.wasPressed(Key::W));
    EXPECT_FALSE(keyboard.wasReleased(Key::W));
}

TEST(KeyboardTest, Apply_LeavesOtherKeysAlone)
{
    Keyboard keyboard;

    keyboard.apply(KeyPressed{.key = Key::W});

    EXPECT_FALSE(keyboard.isDown(Key::A));
    EXPECT_FALSE(keyboard.wasPressed(Key::A));
}

TEST(KeyboardTest, Apply_MarksAReleasedKeyNoLongerHeld)
{
    Keyboard keyboard;
    keyboard.apply(KeyPressed{.key = Key::W});

    keyboard.apply(KeyReleased{.key = Key::W});

    EXPECT_FALSE(keyboard.isDown(Key::W));
    EXPECT_TRUE(keyboard.wasReleased(Key::W));
}

TEST(KeyboardTest, Apply_TreatsARepeatAsHeldButNotAsAFreshPress)
{
    Keyboard keyboard;

    keyboard.apply(KeyPressed{.key = Key::W, .repeat = true});

    EXPECT_TRUE(keyboard.isDown(Key::W));
    EXPECT_FALSE(keyboard.wasPressed(Key::W));
}

TEST(KeyboardTest, Apply_KeepsAKeyPressedWhenARepeatFollowsThePress)
{
    Keyboard keyboard;
    keyboard.apply(KeyPressed{.key = Key::W});

    keyboard.apply(KeyPressed{.key = Key::W, .repeat = true});

    EXPECT_TRUE(keyboard.wasPressed(Key::W));
}

TEST(KeyboardTest, Apply_RecordsTheModifiersAPressWasCarrying)
{
    Keyboard keyboard;

    keyboard.apply(
        KeyPressed{.key = Key::S, .modifiers = {.control = true}});

    EXPECT_TRUE(keyboard.modifiers().control);
    EXPECT_FALSE(keyboard.modifiers().shift);
}

TEST(KeyboardTest, Apply_RecordsTheModifiersAReleaseWasCarrying)
{
    Keyboard keyboard;

    keyboard.apply(
        KeyReleased{.key = Key::S, .modifiers = {.shift = true}});

    EXPECT_TRUE(keyboard.modifiers().shift);
}

TEST(KeyboardTest, ApplyModifiers_ReplacesWhatIsHeld)
{
    Keyboard keyboard;
    keyboard.applyModifiers(KeyModifiers{.alt = true, .super = true});

    keyboard.applyModifiers(KeyModifiers{.alt = true});

    EXPECT_TRUE(keyboard.modifiers().alt);
    EXPECT_FALSE(keyboard.modifiers().super);
}

TEST(KeyboardTest, BeginTick_ClearsTheEdgesButKeepsWhatIsHeld)
{
    Keyboard keyboard;
    keyboard.apply(KeyPressed{.key = Key::W});
    keyboard.apply(KeyPressed{.key = Key::A});
    keyboard.apply(KeyReleased{.key = Key::A});

    keyboard.beginTick();

    EXPECT_TRUE(keyboard.isDown(Key::W));
    EXPECT_FALSE(keyboard.wasPressed(Key::W));
    EXPECT_FALSE(keyboard.wasReleased(Key::A));
}

TEST(KeyboardTest, BeginTick_KeepsTheModifiersHeld)
{
    Keyboard keyboard;
    keyboard.applyModifiers(KeyModifiers{.shift = true});

    keyboard.beginTick();

    EXPECT_TRUE(keyboard.modifiers().shift);
}

TEST(KeyboardTest, ReadingAnEdgeTwiceGivesTheSameAnswer)
{
    Keyboard keyboard;
    keyboard.apply(KeyPressed{.key = Key::Space});

    EXPECT_TRUE(keyboard.wasPressed(Key::Space));
    EXPECT_TRUE(keyboard.wasPressed(Key::Space));
}

TEST(KeyboardTest, Apply_IgnoresAKeyOutsideTheEnumeration)
{
    Keyboard keyboard;

    keyboard.apply(KeyPressed{.key = kUnnamedKey});
    keyboard.apply(KeyReleased{.key = kUnnamedKey});

    EXPECT_FALSE(keyboard.isDown(kUnnamedKey));
    EXPECT_FALSE(keyboard.wasPressed(kUnnamedKey));
    EXPECT_FALSE(keyboard.wasReleased(kUnnamedKey));
    EXPECT_EQ(KeyModifiers{}, keyboard.pressModifiers(kUnnamedKey));
}

// The edge's own modifiers, kept beside the edge.
// modifiers() is the tick's last word and answers a different question.
TEST(KeyboardTest, PressModifiers_KeepsWhatOneKeysPressEdgeCarried)
{
    Keyboard keyboard;

    keyboard.apply(
        KeyPressed{.key = Key::S, .modifiers = {.control = true}});
    keyboard.apply(KeyReleased{.key = Key::LeftControl, .modifiers = {}});

    EXPECT_TRUE(keyboard.pressModifiers(Key::S).control);
    EXPECT_FALSE(keyboard.modifiers().control);
}

TEST(KeyboardTest, PressModifiers_ReportsNoneForAKeyThatDidNotGoDown)
{
    Keyboard keyboard;

    keyboard.apply(
        KeyPressed{.key = Key::W, .modifiers = {.shift = true}});

    EXPECT_EQ(KeyModifiers{}, keyboard.pressModifiers(Key::S));
}

// A repeat is not a press, so it may not restate a press's modifiers.
TEST(KeyboardTest, PressModifiers_IgnoresARepeat)
{
    Keyboard keyboard;

    keyboard.apply(
        KeyPressed{
            .key = Key::W, .modifiers = {.shift = true}, .repeat = true});

    EXPECT_EQ(KeyModifiers{}, keyboard.pressModifiers(Key::W));
}

TEST(KeyboardTest, BeginTick_ForgetsThePressEdgesModifiers)
{
    Keyboard keyboard;
    keyboard.apply(
        KeyPressed{.key = Key::S, .modifiers = {.control = true}});

    keyboard.beginTick();

    EXPECT_EQ(KeyModifiers{}, keyboard.pressModifiers(Key::S));
}

TEST(KeyboardTest, Apply_StillRecordsTheModifiersOfAnUnnamedKey)
{
    Keyboard keyboard;

    keyboard.apply(
        KeyPressed{.key = kUnnamedKey, .modifiers = {.shift = true}});

    EXPECT_TRUE(keyboard.modifiers().shift);
}

TEST(KeyboardTest, Apply_IgnoresAnUnnamedKeyOnRelease)
{
    Keyboard keyboard;

    keyboard.apply(
        KeyReleased{.key = kUnnamedKey, .modifiers = {.alt = true}});

    EXPECT_TRUE(keyboard.modifiers().alt);
    EXPECT_FALSE(keyboard.wasReleased(kUnnamedKey));
}
