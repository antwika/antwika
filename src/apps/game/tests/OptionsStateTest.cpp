#include <gtest/gtest.h>

#include <antwika/input/Key.hpp>

#include "antwika/game/Action.hpp"
#include "antwika/game/KeyBindings.hpp"
#include "antwika/game/OptionsState.hpp"

using antwika::game::Action;
using antwika::game::BindOutcome;
using antwika::game::kDefaultBindings;
using antwika::game::kQuitKey;
using antwika::game::OptionsState;
using antwika::input::Key;

TEST(OptionsStateTest, ARunOpensWithTheScreenDownAndTheShippedLayout)
{
    const OptionsState options;

    EXPECT_FALSE(options.open());
    EXPECT_FALSE(options.awaiting().has_value());
    EXPECT_FALSE(options.notice().has_value());
    EXPECT_EQ(options.bindings(), kDefaultBindings);
}

TEST(OptionsStateTest, TheScreenIsShownAndPutAway)
{
    OptionsState options;

    options.setOpen(true);
    EXPECT_TRUE(options.open());

    options.setOpen(false);
    EXPECT_FALSE(options.open());
}

// A key offered when nothing asked for one is an ordinary state.
// Every key press on the screen arrives here.
// Including the ones pressed while the menu itself is up.
TEST(OptionsStateTest, AKeyNothingAskedForIsAnsweredWithNothing)
{
    OptionsState options;

    EXPECT_FALSE(options.press(Key::J).has_value());
    EXPECT_EQ(options.bindings(), kDefaultBindings);
}

TEST(OptionsStateTest, AKeyOfferedToAWaitingActionBindsIt)
{
    OptionsState options;

    options.await(Action::Pause);
    EXPECT_EQ(options.awaiting(), Action::Pause);

    EXPECT_EQ(options.press(Key::J), BindOutcome::Bound);
    EXPECT_EQ(options.bindings().keyFor(Action::Pause), Key::J);

    // An accepted key ends the gesture.
    EXPECT_FALSE(options.awaiting().has_value());
    EXPECT_EQ(options.notice(), BindOutcome::Bound);
}

TEST(OptionsStateTest, OfferingTheKeyAnActionHoldsEndsTheGestureToo)
{
    OptionsState options;

    options.await(Action::Pause);

    EXPECT_EQ(
        options.press(kDefaultBindings.keyFor(Action::Pause)),
        BindOutcome::Unchanged);
    EXPECT_FALSE(options.awaiting().has_value());
}

// A refusal leaves the question up, so the next key answers it.
TEST(OptionsStateTest, ARefusedKeyLeavesTheActionWaiting)
{
    OptionsState options;

    options.await(Action::Pause);

    EXPECT_EQ(
        options.press(kDefaultBindings.keyFor(Action::ZoomIn)),
        BindOutcome::Taken);
    EXPECT_EQ(options.awaiting(), Action::Pause);
    EXPECT_EQ(options.notice(), BindOutcome::Taken);

    EXPECT_EQ(options.press(kQuitKey), BindOutcome::Reserved);
    EXPECT_EQ(options.awaiting(), Action::Pause);
    EXPECT_EQ(options.notice(), BindOutcome::Reserved);

    EXPECT_EQ(options.press(Key::J), BindOutcome::Bound);
    EXPECT_FALSE(options.awaiting().has_value());
}

// A stale refusal must not sit under a fresh question.
TEST(OptionsStateTest, AskingAgainForgetsWhatTheLastAttemptSaid)
{
    OptionsState options;

    options.await(Action::Pause);
    EXPECT_EQ(options.press(kQuitKey), BindOutcome::Reserved);

    options.await(Action::ZoomIn);
    EXPECT_FALSE(options.notice().has_value());
}

// A screen put away is not still half-way through a gesture.
TEST(OptionsStateTest, PuttingTheScreenAwayForgetsTheWaitingAction)
{
    OptionsState options;

    options.setOpen(true);
    options.await(Action::Pause);
    options.setOpen(false);

    EXPECT_FALSE(options.awaiting().has_value());
    EXPECT_FALSE(options.notice().has_value());
}

// What the machine's announced layout arrives through.
// It is nobody's gesture, so it touches neither of those.
TEST(OptionsStateTest, AnAnnouncedBindingTouchesNoGesture)
{
    OptionsState options;

    options.await(Action::Pause);

    EXPECT_EQ(options.apply(Action::ZoomIn, Key::J), BindOutcome::Bound);
    EXPECT_EQ(options.bindings().keyFor(Action::ZoomIn), Key::J);
    EXPECT_EQ(options.awaiting(), Action::Pause);
    EXPECT_FALSE(options.notice().has_value());
}
