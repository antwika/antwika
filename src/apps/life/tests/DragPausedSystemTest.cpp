#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/ecs/World.hpp>
#include <antwika/ecs/mocks/MockSystem.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/life/DragPausedSystem.hpp"
#include "antwika/life/DragState.hpp"

using antwika::ecs::World;
using antwika::ecs::mocks::MockSystem;
using antwika::life::DragPausedSystem;
using antwika::life::DragState;
using antwika::log::mocks::MockLogger;
using ::testing::NiceMock;
using ::testing::Ref;

namespace
{
    constexpr antwika::time::Tick kTick = 7;
}

TEST(DragPausedSystemTest, Update_RunsTheWrappedSystemWhenNothingIsHeld)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const DragState drag;

    NiceMock<MockSystem> inner;
    EXPECT_CALL(inner, update(Ref(world), kTick));

    DragPausedSystem system(inner, drag);
    system.update(world, kTick);
}

TEST(DragPausedSystemTest, Update_HoldsTheWrappedSystemStillDuringADrag)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    DragState drag;
    drag.begin();

    NiceMock<MockSystem> inner;
    EXPECT_CALL(inner, update).Times(0);

    DragPausedSystem system(inner, drag);
    system.update(world, kTick);
}

TEST(DragPausedSystemTest, Update_RunsAgainOnceTheDragIsOver)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    DragState drag;

    NiceMock<MockSystem> inner;
    EXPECT_CALL(inner, update).Times(2);

    DragPausedSystem system(inner, drag);

    system.update(world, 0);

    drag.begin();
    system.update(world, 1);
    system.update(world, 2);

    drag.end();
    system.update(world, 3);
}

TEST(DragPausedSystemTest, Update_PassesTheTickThroughUnchanged)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const DragState drag;

    NiceMock<MockSystem> inner;
    EXPECT_CALL(inner, update(Ref(world), 0));
    EXPECT_CALL(inner, update(Ref(world), 41));

    DragPausedSystem system(inner, drag);
    system.update(world, 0);
    system.update(world, 41);
}
