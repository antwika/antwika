#include "antwika/task_worker/Worker.hpp"

#include <gtest/gtest.h>

#include <antwika/ecs/World.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

using antwika::ecs::World;
using antwika::log::mocks::MockLogger;
using antwika::task_worker::makeWorkerLabel;
using antwika::task_worker::Worker;
using antwika::task_worker::WorkerStatus;
using ::testing::NiceMock;

TEST(WorkerTest, EqualityComparesEveryFieldIndependently)
{
    const Worker base{
        WorkerStatus::Busy, 3, 7, makeWorkerLabel("Render")};

    EXPECT_NE(
        base,
        (Worker{WorkerStatus::Idle, 3, 7, makeWorkerLabel("Render")}));
    EXPECT_NE(
        base,
        (Worker{WorkerStatus::Busy, 4, 7, makeWorkerLabel("Render")}));
    EXPECT_NE(
        base,
        (Worker{WorkerStatus::Busy, 3, 8, makeWorkerLabel("Render")}));
    EXPECT_NE(
        base,
        (Worker{WorkerStatus::Busy, 3, 7, makeWorkerLabel("Other")}));
    EXPECT_EQ(
        base,
        (Worker{WorkerStatus::Busy, 3, 7, makeWorkerLabel("Render")}));
}

TEST(WorkerTest, WorkerComponentCanBeRemovedFromAnEntity)
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
