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

TEST(OptionsStateTest, Open_StartsDownWithTheShippedLayout)
{
    const OptionsState options;

    EXPECT_FALSE(options.open());
    EXPECT_FALSE(options.awaiting().has_value());
    EXPECT_FALSE(options.notice().has_value());
    EXPECT_EQ(options.bindings(), kDefaultBindings);
}

TEST(OptionsStateTest, SetOpen_ShowsAndPutsTheScreenAway)
{
    OptionsState options;

    options.setOpen(true);
    EXPECT_TRUE(options.open());

    options.setOpen(false);
    EXPECT_FALSE(options.open());
}

TEST(OptionsStateTest, Press_AnswersNothingWhenNoneWasAsked)
{
    OptionsState options;

    EXPECT_FALSE(options.press(Key::J).has_value());
    EXPECT_EQ(options.bindings(), kDefaultBindings);
}

TEST(OptionsStateTest, Press_BindsAKeyOfferedToAWaitingAction)
{
    OptionsState options;

    options.await(Action::Pause);
    EXPECT_EQ(options.awaiting(), Action::Pause);

    EXPECT_EQ(options.press(Key::J), BindOutcome::Bound);
    EXPECT_EQ(options.bindings().keyFor(Action::Pause), Key::J);

    EXPECT_FALSE(options.awaiting().has_value());
    EXPECT_EQ(options.notice(), BindOutcome::Bound);
}

TEST(OptionsStateTest, Press_EndsTheGestureOnItsOwnKey)
{
    OptionsState options;

    options.await(Action::Pause);

    EXPECT_EQ(
        options.press(kDefaultBindings.keyFor(Action::Pause)),
        BindOutcome::Unchanged);
    EXPECT_FALSE(options.awaiting().has_value());
}

TEST(OptionsStateTest, Press_LeavesTheActionWaitingOnRefusal)
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

TEST(OptionsStateTest, Await_ForgetsTheLastAttempt)
{
    OptionsState options;

    options.await(Action::Pause);
    EXPECT_EQ(options.press(kQuitKey), BindOutcome::Reserved);

    options.await(Action::ZoomIn);
    EXPECT_FALSE(options.notice().has_value());
}

TEST(OptionsStateTest, SetOpen_ForgetsTheWaitingAction)
{
    OptionsState options;

    options.setOpen(true);
    options.await(Action::Pause);
    options.setOpen(false);

    EXPECT_FALSE(options.awaiting().has_value());
    EXPECT_FALSE(options.notice().has_value());
}

TEST(OptionsStateTest, Apply_TouchesNoGesture)
{
    OptionsState options;

    options.await(Action::Pause);

    EXPECT_EQ(options.apply(Action::ZoomIn, Key::J), BindOutcome::Bound);
    EXPECT_EQ(options.bindings().keyFor(Action::ZoomIn), Key::J);
    EXPECT_EQ(options.awaiting(), Action::Pause);
    EXPECT_FALSE(options.notice().has_value());
}
