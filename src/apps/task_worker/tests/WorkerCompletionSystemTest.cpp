#include <gtest/gtest.h>

#include <antwika/ecs_commons/Name.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/scheduler/JobId.hpp>
#include <antwika/scheduler/Priority.hpp>

#include "antwika/task_worker/WorkerCompletionSystem.hpp"
#include "antwika/task_worker/TaskRegistry.hpp"
#include "antwika/task_worker/Worker.hpp"

using antwika::ecs::World;
using antwika::ecs_commons::makeName;
using antwika::log::mocks::MockLogger;
using antwika::scheduler::JobId;
using antwika::scheduler::kNormalPriority;
using antwika::task_worker::TaskRegistry;
using antwika::task_worker::TaskStatus;
using antwika::task_worker::Worker;
using antwika::task_worker::WorkerCompletionSystem;
using antwika::task_worker::WorkerStatus;
using ::testing::NiceMock;

TEST(WorkerCompletionSystemTest, Update_IdlesAWorkerOnTheLastTick)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto entity = world.create();
    world.add<Worker>(entity, Worker{WorkerStatus::Busy, 1});
    world.commit();

    TaskRegistry registry;
    WorkerCompletionSystem system(registry);
    system.update(world, 0);
    world.commit();

    EXPECT_EQ(world.get<Worker>(entity), (Worker{WorkerStatus::Idle, 0}));
}

TEST(WorkerCompletionSystemTest, Update_KeepsAWorkerBusyWithTicksLeft)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto entity = world.create();
    world.add<Worker>(entity, Worker{WorkerStatus::Busy, 2});
    world.commit();

    TaskRegistry registry;
    WorkerCompletionSystem system(registry);
    system.update(world, 0);
    world.commit();

    EXPECT_EQ(world.get<Worker>(entity), (Worker{WorkerStatus::Busy, 1}));
}

TEST(WorkerCompletionSystemTest, Update_LeavesAnIdleWorkerAlone)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto entity = world.create();
    world.add<Worker>(entity, Worker{WorkerStatus::Idle, 0});
    world.commit();

    TaskRegistry registry;
    WorkerCompletionSystem system(registry);
    system.update(world, 0);
    world.commit();

    EXPECT_EQ(world.get<Worker>(entity), (Worker{WorkerStatus::Idle, 0}));
}

TEST(
    WorkerCompletionSystemTest,
    Update_MarksTheTaskCompletedOnTheLastTick)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto entity = world.create();
    world.add<Worker>(
        entity,
        Worker{WorkerStatus::Busy, 1, 7, makeName("Render")});
    world.commit();

    TaskRegistry registry;
    registry.submit(7, "Render", kNormalPriority, 1);
    registry.markStarted(static_cast<JobId>(1));
    WorkerCompletionSystem system(registry);
    system.update(world, 0);
    world.commit();

    const auto &task = registry.allTasks()[0];
    EXPECT_EQ(task.status, TaskStatus::Completed);
    EXPECT_EQ(task.remainingTicks, 0U);
}

TEST(
    WorkerCompletionSystemTest,
    Update_UpdatesRemainingWhileStillRunning)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto entity = world.create();
    world.add<Worker>(
        entity,
        Worker{WorkerStatus::Busy, 2, 7, makeName("Render")});
    world.commit();

    TaskRegistry registry;
    registry.submit(7, "Render", kNormalPriority, 2);
    registry.markStarted(static_cast<JobId>(1));
    WorkerCompletionSystem system(registry);
    system.update(world, 0);
    world.commit();

    const auto &task = registry.allTasks()[0];
    EXPECT_EQ(task.status, TaskStatus::Running);
    EXPECT_EQ(task.remainingTicks, 1U);
}
