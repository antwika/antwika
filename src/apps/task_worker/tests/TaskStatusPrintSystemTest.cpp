#include "antwika/task_worker/TaskStatusPrintSystem.hpp"

#include <sstream>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/scheduler/JobId.hpp>
#include <antwika/scheduler/Priority.hpp>

using antwika::ecs::World;
using antwika::log::mocks::MockLogger;
using antwika::scheduler::JobId;
using antwika::scheduler::kCriticalPriority;
using antwika::scheduler::kLowPriority;
using antwika::scheduler::kNormalPriority;
using antwika::task_worker::TaskRegistry;
using antwika::task_worker::TaskStatusPrintSystem;
using ::testing::NiceMock;

TEST(TaskStatusPrintSystemTest, PrintsNothingWhenNoTaskHasBeenSubmitted)
{
    NiceMock<MockLogger> logger;
    World world(logger);

    TaskRegistry registry;
    std::ostringstream out;
    TaskStatusPrintSystem system(out, registry);

    system.update(world, 0);

    EXPECT_EQ(out.str(), "Tasks after tick 0:\n");
}

TEST(TaskStatusPrintSystemTest, PrintsAPendingTaskWithItsFullDurationLeft)
{
    NiceMock<MockLogger> logger;
    World world(logger);

    TaskRegistry registry;
    registry.submit(3, "Gamma", kLowPriority, 1);

    std::ostringstream out;
    TaskStatusPrintSystem system(out, registry);

    system.update(world, 0);

    EXPECT_EQ(
        out.str(),
        "Tasks after tick 0:\n"
        "  Task id: 3 | Task name: Gamma | Priority: 0 | "
        "Status: Pending | Remaining: 1 tick(s)\n");
}

TEST(TaskStatusPrintSystemTest, PrintsARunningTaskWithItsLiveCountdown)
{
    NiceMock<MockLogger> logger;
    World world(logger);

    TaskRegistry registry;
    registry.submit(4, "Delta", kCriticalPriority, 1);
    registry.markStarted(static_cast<JobId>(1));
    registry.updateRemaining(4, 1);

    std::ostringstream out;
    TaskStatusPrintSystem system(out, registry);

    system.update(world, 4);

    EXPECT_EQ(
        out.str(),
        "Tasks after tick 4:\n"
        "  Task id: 4 | Task name: Delta | Priority: 3 | "
        "Status: Running | Remaining: 1 tick(s)\n");
}

TEST(TaskStatusPrintSystemTest, PrintsACompletedTaskWithZeroTicksLeft)
{
    NiceMock<MockLogger> logger;
    World world(logger);

    TaskRegistry registry;
    registry.submit(1, "Alpha", kNormalPriority, 4);
    registry.markStarted(static_cast<JobId>(1));
    registry.markCompleted(1);

    std::ostringstream out;
    TaskStatusPrintSystem system(out, registry);

    system.update(world, 4);

    EXPECT_EQ(
        out.str(),
        "Tasks after tick 4:\n"
        "  Task id: 1 | Task name: Alpha | Priority: 1 | "
        "Status: Completed | Remaining: 0 tick(s)\n");
}

TEST(
    TaskStatusPrintSystemTest,
    PrintsEveryTaskInSubmissionOrderRegardlessOfStatus)
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
    TaskStatusPrintSystem system(out, registry);

    system.update(world, 4);

    EXPECT_EQ(
        out.str(),
        "Tasks after tick 4:\n"
        "  Task id: 1 | Task name: Alpha | Priority: 1 | "
        "Status: Completed | Remaining: 0 tick(s)\n"
        "  Task id: 2 | Task name: Beta | Priority: 1 | "
        "Status: Running | Remaining: 5 tick(s)\n"
        "  Task id: 3 | Task name: Gamma | Priority: 0 | "
        "Status: Pending | Remaining: 1 tick(s)\n");
}
