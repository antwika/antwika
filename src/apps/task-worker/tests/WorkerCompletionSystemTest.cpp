#include "antwika/task-worker/WorkerCompletionSystem.hpp"

#include <gtest/gtest.h>

#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/task-worker/Worker.hpp"

using antwika::ecs::World;
using antwika::log::mocks::MockLogger;
using antwika::task_worker::Worker;
using antwika::task_worker::WorkerCompletionSystem;
using antwika::task_worker::WorkerStatus;
using ::testing::NiceMock;

TEST(WorkerCompletionSystemTest, CountdownOneBecomesIdleAfterOneUpdate)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto entity = world.create();
    world.add<Worker>(entity, Worker{WorkerStatus::Busy, 1});
    world.commit();

    WorkerCompletionSystem system;
    system.update(world, 0);
    world.commit();

    EXPECT_EQ(world.get<Worker>(entity), (Worker{WorkerStatus::Idle, 0}));
}

TEST(WorkerCompletionSystemTest, CountdownTwoStaysBusyAfterOneUpdate)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto entity = world.create();
    world.add<Worker>(entity, Worker{WorkerStatus::Busy, 2});
    world.commit();

    WorkerCompletionSystem system;
    system.update(world, 0);
    world.commit();

    EXPECT_EQ(world.get<Worker>(entity), (Worker{WorkerStatus::Busy, 1}));
}

TEST(WorkerCompletionSystemTest, IdleWorkerIsUnaffected)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto entity = world.create();
    world.add<Worker>(entity, Worker{WorkerStatus::Idle, 0});
    world.commit();

    WorkerCompletionSystem system;
    system.update(world, 0);
    world.commit();

    EXPECT_EQ(world.get<Worker>(entity), (Worker{WorkerStatus::Idle, 0}));
}
