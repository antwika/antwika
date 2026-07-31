#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/game/AppMode.hpp"
#include "antwika/game/ModeGatedSystem.hpp"
#include "antwika/game/PauseGatedSystem.hpp"
#include "antwika/game/PauseState.hpp"

using antwika::ecs::ISystem;
using antwika::ecs::World;
using antwika::game::AppMode;
using antwika::game::AppModeState;
using antwika::game::ModeGatedSystem;
using antwika::game::PauseGatedSystem;
using antwika::game::PauseState;
using antwika::log::mocks::MockLogger;
using ::testing::NiceMock;

namespace
{
    // A counter rather than a mock, as ModeGateTest's is.
    // What is asserted is how many times something ran.
    class CountingSystem final : public ISystem
    {
    public:
        void update(World &, antwika::time::Tick) override
        {
            ++calls;
        }

        std::size_t calls = 0;
    };

    class PauseGatedSystemTest : public ::testing::Test
    {
    protected:
        NiceMock<MockLogger> logger;
        World world{logger};
        CountingSystem inner;
        PauseState pause;
    };
} // namespace

TEST_F(PauseGatedSystemTest, Update_RunsTheSystemWhileNothingIsPaused)
{
    PauseGatedSystem gate(inner, pause);

    gate.update(world, 0);

    EXPECT_EQ(inner.calls, 1U);
}

TEST_F(PauseGatedSystemTest, Update_StagesNothingWhilePaused)
{
    PauseGatedSystem gate(inner, pause);
    pause.toggle();

    gate.update(world, 0);

    EXPECT_EQ(inner.calls, 0U);
}

// The pause is a state, not an edge: every tick under it is held.
TEST_F(PauseGatedSystemTest, Update_RunsAgainOnceTheRunIsResumed)
{
    PauseGatedSystem gate(inner, pause);

    pause.toggle();
    gate.update(world, 0);
    gate.update(world, 1);
    pause.toggle();
    gate.update(world, 2);

    EXPECT_EQ(inner.calls, 1U);
}

// bootstrap() puts one gate over the other, so both have to agree.
TEST_F(PauseGatedSystemTest, Update_StagesNothingWhenEitherGateSaysNo)
{
    const AppModeState menu{AppMode::MainMenu};
    ModeGatedSystem modeGate(inner, menu, AppMode::CityMap);
    PauseGatedSystem gate(modeGate, pause);

    gate.update(world, 0);

    EXPECT_EQ(inner.calls, 0U);
}

TEST_F(PauseGatedSystemTest, Update_RunsWhenBothGatesAgree)
{
    const AppModeState city{AppMode::CityMap};
    ModeGatedSystem modeGate(inner, city, AppMode::CityMap);
    PauseGatedSystem gate(modeGate, pause);

    gate.update(world, 0);

    EXPECT_EQ(inner.calls, 1U);
}
