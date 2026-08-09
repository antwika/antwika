#include <gtest/gtest.h>

#include <antwika/ecs_commons/Name.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/task_worker/TaskJob.hpp"
#include "antwika/task_worker/Worker.hpp"
#include "antwika/task_worker/WorkerLookup.hpp"

using antwika::ecs::World;
using antwika::ecs_commons::makeName;
using antwika::log::mocks::MockLogger;
using antwika::task_worker::TaskJob;
using antwika::task_worker::Worker;
using antwika::task_worker::WorkerLookup;
using antwika::task_worker::WorkerStatus;
using ::testing::NiceMock;

TEST(TaskJobTest, Execute_ClaimsTheLowestIndexIdleWorker)
{
    NiceMock<MockLogger> logger;
    World world(logger);

    const auto busy = world.create();
    world.add<Worker>(busy, Worker{WorkerStatus::Busy, 3});
    const auto idleFirst = world.create();
    world.add<Worker>(idleFirst, Worker{WorkerStatus::Idle, 0});
    const auto idleSecond = world.create();
    world.add<Worker>(idleSecond, Worker{WorkerStatus::Idle, 0});
    world.commit();

    WorkerLookup lookup(world, {busy, idleFirst, idleSecond});
    lookup.refresh();

    TaskJob job(lookup, 1, "Task", 5);
    job.execute(0);
    world.commit();

    EXPECT_EQ(world.get<Worker>(busy), (Worker{WorkerStatus::Busy, 3}));
    EXPECT_EQ(
        world.get<Worker>(idleFirst),
        (Worker{WorkerStatus::Busy, 5, 1, makeName("Task")}));
    EXPECT_EQ(
        world.get<Worker>(idleSecond), (Worker{WorkerStatus::Idle, 0}));
    EXPECT_NE(
        world.get<Worker>(idleFirst), (Worker{WorkerStatus::Idle, 0}));
    EXPECT_NE(
        world.get<Worker>(idleFirst), (Worker{WorkerStatus::Busy, 99}));
}
