#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/ecs/World.hpp>
#include <antwika/ecs/mocks/MockSystem.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/ecs_commons/EcsCommonsError.hpp"
#include "antwika/ecs_commons/PeriodicSystem.hpp"

using antwika::ecs::World;
using antwika::ecs::mocks::MockSystem;
using antwika::ecs_commons::EcsCommonsError;
using antwika::ecs_commons::PeriodicSystem;
using antwika::log::mocks::MockLogger;
using ::testing::NiceMock;

TEST(PeriodicSystemTest, RunsTheInnerSystemOnEveryNthTick)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    MockSystem inner;
    PeriodicSystem system(inner, 3);

    EXPECT_CALL(inner, update(::testing::_, 0)).Times(1);
    EXPECT_CALL(inner, update(::testing::_, 3)).Times(1);
    EXPECT_CALL(inner, update(::testing::_, 6)).Times(1);

    for (antwika::time::Tick tick = 0; tick < 7; ++tick)
    {
        system.update(world, tick);
    }
}

TEST(PeriodicSystemTest, APeriodOfOneIsDueEveryTick)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    MockSystem inner;
    PeriodicSystem system(inner, 1);

    EXPECT_CALL(inner, update(::testing::_, ::testing::_)).Times(4);

    for (antwika::time::Tick tick = 0; tick < 4; ++tick)
    {
        system.update(world, tick);
    }
}

TEST(PeriodicSystemTest, TheOffsetPicksWhichTickWithinThePeriodIsDue)
{
    MockSystem inner;
    const PeriodicSystem system(inner, 4, 1);

    EXPECT_FALSE(system.due(0));
    EXPECT_TRUE(system.due(1));
    EXPECT_FALSE(system.due(2));
    EXPECT_TRUE(system.due(5));
}

TEST(PeriodicSystemTest, AnOffsetBeyondThePeriodWrapsIntoIt)
{
    MockSystem inner;
    const PeriodicSystem system(inner, 4, 9);

    EXPECT_TRUE(system.due(1));
    EXPECT_FALSE(system.due(0));
}

TEST(PeriodicSystemTest, RejectsAPeriodOfZero)
{
    MockSystem inner;

    EXPECT_THROW(PeriodicSystem(inner, 0), EcsCommonsError);
}
