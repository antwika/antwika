#include <gtest/gtest.h>

#include <antwika/ecs/World.hpp>
#include <antwika/ecs_commons/Name.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/task_worker/Worker.hpp"

using antwika::ecs::World;
using antwika::ecs_commons::makeName;
using antwika::log::mocks::MockLogger;
using antwika::task_worker::Worker;
using antwika::task_worker::WorkerStatus;
using ::testing::NiceMock;

TEST(WorkerTest, OperatorEquals_ComparesEveryFieldIndependently)
{
    const Worker base{
        WorkerStatus::Busy, 3, 7, makeName("Render")};

    EXPECT_NE(
        base,
        (Worker{WorkerStatus::Idle, 3, 7, makeName("Render")}));
    EXPECT_NE(
        base,
        (Worker{WorkerStatus::Busy, 4, 7, makeName("Render")}));
    EXPECT_NE(
        base,
        (Worker{WorkerStatus::Busy, 3, 8, makeName("Render")}));
    EXPECT_NE(
        base,
        (Worker{WorkerStatus::Busy, 3, 7, makeName("Other")}));
    EXPECT_EQ(
        base,
        (Worker{WorkerStatus::Busy, 3, 7, makeName("Render")}));
}

TEST(WorkerTest, Remove_TakesTheComponentOffAnEntity)
{
    NiceMock<MockLogger> logger;
    World world(logger);

    const auto entity = world.create();
    world.add<Worker>(entity, Worker{WorkerStatus::Idle, 0});
    world.commit();

    world.destroy(entity);
    world.commit();

    EXPECT_FALSE(world.has<Worker>(entity));
}
