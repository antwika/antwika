#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <sstream>
#include <string>

#include <antwika/ecs_commons/Name.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/scheduler/JobId.hpp>
#include <antwika/scheduler/Priority.hpp>

#include "antwika/task_worker/StatusPrintSystem.hpp"
#include "antwika/task_worker/TaskRegistry.hpp"
#include "antwika/task_worker/Worker.hpp"

using antwika::ecs::World;
using antwika::ecs_commons::kNameMaxLength;
using antwika::ecs_commons::makeName;
using antwika::log::mocks::MockLogger;
using antwika::scheduler::JobId;
using antwika::scheduler::kCriticalPriority;
using antwika::scheduler::kLowPriority;
using antwika::scheduler::kNormalPriority;
using antwika::task_worker::StatusPrintSystem;
using antwika::task_worker::TaskDependency;
using antwika::task_worker::TaskRegistry;
using antwika::task_worker::Worker;
using antwika::task_worker::WorkerStatus;
using ::testing::NiceMock;

TEST(StatusPrintSystemTest, Update_PrintsEmptySectionsWhenNothingExists)
{
    NiceMock<MockLogger> logger;
    World world(logger);

    TaskRegistry registry;
    std::ostringstream out;
    StatusPrintSystem system(out, registry);

    system.update(world, 0);

    EXPECT_EQ(
        out.str(),
        "After tick 0:\n"
        "  Tasks:\n"
        "  Workers:\n");
}

TEST(StatusPrintSystemTest, Update_PrintsEveryTaskInSubmissionOrder)
{
    NiceMock<MockLogger> logger;
    World world(logger);

    TaskRegistry registry;
    registry.submit(1, "Alpha", kNormalPriority, 4);
    registry.submit(2, "Beta", kNormalPriority, 5);
    registry.submit(3, "Gamma", kLowPriority, 1);
    registry.markStarted(static_cast<JobId>(1));
    registry.markStarted(static_cast<JobId>(2));
    registry.markCompleted(1);

    std::ostringstream out;
    StatusPrintSystem system(out, registry);

    system.update(world, 4);

    EXPECT_EQ(
        out.str(),
        "After tick 4:\n"
        "  Tasks:\n"
        "    Task id: 1 | Task name: Alpha | Priority: 1 | "
        "Status: Completed | Remaining: 0 tick(s)\n"
        "    Task id: 2 | Task name: Beta | Priority: 1 | "
        "Status: Running | Remaining: 5 tick(s)\n"
        "    Task id: 3 | Task name: Gamma | Priority: 0 | "
        "Status: Pending | Remaining: 1 tick(s)\n"
        "  Workers:\n");
}

TEST(StatusPrintSystemTest, Update_PrintsARunningTasksCountdown)
{
    NiceMock<MockLogger> logger;
    World world(logger);

    TaskRegistry registry;
    registry.submit(4, "Delta", kCriticalPriority, 1);
    registry.markStarted(static_cast<JobId>(1));
    registry.updateRemaining(4, 1);

    std::ostringstream out;
    StatusPrintSystem system(out, registry);

    system.update(world, 4);

    EXPECT_EQ(
        out.str(),
        "After tick 4:\n"
        "  Tasks:\n"
        "    Task id: 4 | Task name: Delta | Priority: 3 | "
        "Status: Running | Remaining: 1 tick(s)\n"
        "  Workers:\n");
}

TEST(StatusPrintSystemTest, Update_PrintsATasksDependencyWhenPresent)
{
    NiceMock<MockLogger> logger;
    World world(logger);

    TaskRegistry registry;
    registry.submit(4, "Delta", kCriticalPriority, 1);
    registry.submit(
        5, "Epsilon", kNormalPriority, 1, TaskDependency{4, "Delta"});
    registry.markStarted(static_cast<JobId>(1));

    std::ostringstream out;
    StatusPrintSystem system(out, registry);

    system.update(world, 4);

    EXPECT_EQ(
        out.str(),
        "After tick 4:\n"
        "  Tasks:\n"
        "    Task id: 4 | Task name: Delta | Priority: 3 | "
        "Status: Running | Remaining: 1 tick(s)\n"
        "    Task id: 5 | Task name: Epsilon | Priority: 1 | "
        "Status: Pending | Remaining: 1 tick(s) | Depends on: Delta (4)\n"
        "  Workers:\n");
}

TEST(StatusPrintSystemTest, Update_PrintsEveryWorkersCurrentState)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto idle = world.create();
    world.add<Worker>(idle, Worker{WorkerStatus::Idle, 0});
    const auto busy = world.create();
    world.add<Worker>(
        busy,
        Worker{WorkerStatus::Busy, 3, 42, makeName("Render")});
    world.commit();

    TaskRegistry registry;
    std::ostringstream out;
    StatusPrintSystem system(out, registry);

    system.update(world, 5);

    EXPECT_EQ(
        out.str(),
        "After tick 5:\n"
        "  Tasks:\n"
        "  Workers:\n"
        "    worker[0] - Current state: Idle\n"
        "    worker[1] - Current state: Busy | Remaining: 3 tick(s) | "
        "Task id: 42 | Task name: Render\n");
}

TEST(StatusPrintSystemTest, Update_PrintsALabelFillingItsBuffer)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const std::string full(kNameMaxLength, 'x');
    const auto busy = world.create();
    world.add<Worker>(
        busy, Worker{WorkerStatus::Busy, 3, 42, makeName(full)});
    world.commit();

    TaskRegistry registry;
    std::ostringstream out;
    StatusPrintSystem system(out, registry);

    system.update(world, 5);

    EXPECT_EQ(
        out.str(),
        "After tick 5:\n"
        "  Tasks:\n"
        "  Workers:\n"
        "    worker[0] - Current state: Busy | Remaining: 3 tick(s) | "
        "Task id: 42 | Task name: " +
            full + "\n");
}

TEST(StatusPrintSystemTest, Update_PrintsBothUnderOneTickHeader)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto worker = world.create();
    world.add<Worker>(
        worker,
        Worker{WorkerStatus::Busy, 1, 4, makeName("Delta")});
    world.commit();

    TaskRegistry registry;
    registry.submit(4, "Delta", kCriticalPriority, 1);
    registry.markStarted(static_cast<JobId>(1));

    std::ostringstream out;
    StatusPrintSystem system(out, registry);

    system.update(world, 4);

    EXPECT_EQ(
        out.str(),
        "After tick 4:\n"
        "  Tasks:\n"
        "    Task id: 4 | Task name: Delta | Priority: 3 | "
        "Status: Running | Remaining: 1 tick(s)\n"
        "  Workers:\n"
        "    worker[0] - Current state: Busy | Remaining: 1 tick(s) | "
        "Task id: 4 | Task name: Delta\n");
}
