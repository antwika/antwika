#include <gtest/gtest.h>

#include <antwika/input/InputEvent.hpp>
#include <antwika/input/InputState.hpp>
#include <antwika/input/Key.hpp>

#include "antwika/game/app/Actions.hpp"

using antwika::game::getDefaultActions;
using antwika::game::kLeave;
using antwika::game::kRun;
using antwika::game::kWalkEast;
using antwika::game::kWalkNorth;
using antwika::game::kWalkSouth;
using antwika::game::kWalkWest;
using antwika::input::InputState;
using antwika::input::Key;
using antwika::input::KeyPressed;
using antwika::input::KeyReleased;

namespace
{

    void press(InputState &state, const Key key)
    {
        state.apply(KeyPressed{.key = key});
    }

    void release(InputState &state, const Key key)
    {
        state.apply(KeyReleased{.key = key});
    }

    TEST(ActionsTest, DefaultActions_SendsTheWalkerEveryWayByLetter)
    {
        const auto actions = getDefaultActions();
        InputState state;

        press(state, Key::W);
        press(state, Key::S);
        press(state, Key::A);
        press(state, Key::D);

        EXPECT_TRUE(actions.isActive(kWalkNorth, state));
        EXPECT_TRUE(actions.isActive(kWalkSouth, state));
        EXPECT_TRUE(actions.isActive(kWalkWest, state));
        EXPECT_TRUE(actions.isActive(kWalkEast, state));
    }

    TEST(ActionsTest, DefaultActions_SendsTheWalkerEveryWayByArrow)
    {
        const auto actions = getDefaultActions();
        InputState state;

        press(state, Key::ArrowUp);
        press(state, Key::ArrowDown);
        press(state, Key::ArrowLeft);
        press(state, Key::ArrowRight);

        EXPECT_TRUE(actions.isActive(kWalkNorth, state));
        EXPECT_TRUE(actions.isActive(kWalkSouth, state));
        EXPECT_TRUE(actions.isActive(kWalkWest, state));
        EXPECT_TRUE(actions.isActive(kWalkEast, state));
    }

    TEST(ActionsTest, DefaultActions_LetsAWayGoWhenTheKeyIsLetGo)
    {
        const auto actions = getDefaultActions();
        InputState state;

        press(state, Key::W);
        ASSERT_TRUE(actions.isActive(kWalkNorth, state));

        release(state, Key::W);

        EXPECT_FALSE(actions.isActive(kWalkNorth, state));
    }

    TEST(ActionsTest, DefaultActions_AsksToLeaveOnEscapeStruck)
    {
        const auto actions = getDefaultActions();
        InputState state;

        press(state, Key::Escape);

        EXPECT_TRUE(actions.wasTriggered(kLeave, state));

        state.beginTick();

        EXPECT_FALSE(actions.wasTriggered(kLeave, state));
    }

    TEST(ActionsTest, DefaultActions_AsksToRunWhileEitherShiftIsHeld)
    {
        const auto actions = getDefaultActions();
        InputState state;

        press(state, Key::LeftShift);
        EXPECT_TRUE(actions.isActive(kRun, state));

        release(state, Key::LeftShift);
        EXPECT_FALSE(actions.isActive(kRun, state));

        press(state, Key::RightShift);
        EXPECT_TRUE(actions.isActive(kRun, state));
    }

    TEST(ActionsTest, DefaultActions_LeavesAKeyOfNoConcernAlone)
    {
        const auto actions = getDefaultActions();
        InputState state;

        press(state, Key::Q);

        EXPECT_FALSE(actions.isActive(kWalkNorth, state));
        EXPECT_FALSE(actions.isActive(kWalkSouth, state));
        EXPECT_FALSE(actions.isActive(kWalkWest, state));
        EXPECT_FALSE(actions.isActive(kWalkEast, state));
        EXPECT_FALSE(actions.isActive(kRun, state));
        EXPECT_FALSE(actions.wasTriggered(kLeave, state));
    }

    TEST(ActionsTest, DefaultActions_WalksOnWhileTheRunKeyIsHeldToo)
    {
        const auto actions = getDefaultActions();
        InputState state;

        press(state, Key::LeftShift);
        press(state, Key::W);

        EXPECT_TRUE(actions.isActive(kRun, state));
        EXPECT_TRUE(actions.isActive(kWalkNorth, state));
    }

}
