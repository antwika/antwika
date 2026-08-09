#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/ecs/mocks/MockSystem.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/game/AppMode.hpp"
#include "antwika/game/SessionGatedSystem.hpp"
#include "antwika/game/PauseGatedSystem.hpp"
#include "antwika/game/PauseState.hpp"

using antwika::ecs::ISystem;
using antwika::ecs::World;
using antwika::ecs::mocks::MockSystem;
using antwika::game::AppMode;
using antwika::game::AppModeState;
using antwika::game::SessionGatedSystem;
using antwika::game::PauseGatedSystem;
using antwika::game::PauseState;
using antwika::log::mocks::MockLogger;
using ::testing::_;
using ::testing::NiceMock;

namespace
{
    class PauseGatedSystemTest : public ::testing::Test
    {
    protected:
        NiceMock<MockLogger> logger;
        World world{logger};
        NiceMock<MockSystem> inner;
        PauseState pause;
    };
}

TEST_F(PauseGatedSystemTest, Update_RunsTheSystemWhileNothingIsPaused)
{
    EXPECT_CALL(inner, update(_, _)).Times(1);

    PauseGatedSystem gate(inner, pause);

    gate.update(world, 0);
}

TEST_F(PauseGatedSystemTest, Update_StagesNothingWhilePaused)
{
    EXPECT_CALL(inner, update(_, _)).Times(0);

    PauseGatedSystem gate(inner, pause);
    pause.set(true);

    gate.update(world, 0);
}

TEST_F(PauseGatedSystemTest, Update_RunsAgainOnceTheRunIsResumed)
{
    EXPECT_CALL(inner, update(_, _)).Times(1);

    PauseGatedSystem gate(inner, pause);

    pause.set(true);
    gate.update(world, 0);
    gate.update(world, 1);
    pause.set(false);
    gate.update(world, 2);
}

TEST_F(PauseGatedSystemTest, Update_StagesNothingWhenEitherGateSaysNo)
{
    EXPECT_CALL(inner, update(_, _)).Times(0);

    const AppModeState menu{AppMode::MainMenu};
    SessionGatedSystem sessionGate(inner, menu);
    PauseGatedSystem gate(sessionGate, pause);

    gate.update(world, 0);
}

TEST_F(PauseGatedSystemTest, Update_RunsWhenBothGatesAgree)
{
    EXPECT_CALL(inner, update(_, _)).Times(1);

    const AppModeState city{AppMode::CityMap};
    SessionGatedSystem sessionGate(inner, city);
    PauseGatedSystem gate(sessionGate, pause);

    gate.update(world, 0);
}
