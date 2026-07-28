#include "antwika/task_worker/TaskRegistry.hpp"

#include <optional>

#include <gtest/gtest.h>

#include <antwika/scheduler/Priority.hpp>

using antwika::scheduler::JobId;
using antwika::scheduler::kHighPriority;
using antwika::scheduler::kNormalPriority;
using antwika::task_worker::TaskDependency;
using antwika::task_worker::TaskInfo;
using antwika::task_worker::TaskRegistry;
using antwika::task_worker::TaskStatus;

TEST(TaskRegistryTest, SubmitRecordsAPendingTaskWithDurationAsTicksLeft)
{
    TaskRegistry registry;
    registry.submit(1, "Alpha", kNormalPriority, 5);

    EXPECT_EQ(
        registry.allTasks(),
        (std::vector<TaskInfo>{TaskInfo{
            1, "Alpha", kNormalPriority, TaskStatus::Pending, 5,
            std::nullopt}}));
}

TEST(TaskRegistryTest, AllTasksReportsSubmittedTasksInOrder)
{
    TaskRegistry registry;
    registry.submit(1, "Alpha", kNormalPriority, 5);
    registry.submit(2, "Beta", kHighPriority, 3);

    EXPECT_EQ(
        registry.allTasks(),
        (std::vector<TaskInfo>{
            TaskInfo{
                1, "Alpha", kNormalPriority, TaskStatus::Pending, 5,
                std::nullopt},
            TaskInfo{
                2, "Beta", kHighPriority, TaskStatus::Pending, 3,
                std::nullopt}}));
}

TEST(TaskRegistryTest, SubmitRecordsTheDependedOnTasksIdentity)
{
    TaskRegistry registry;
    registry.submit(4, "Delta", kNormalPriority, 1);
    registry.submit(
        5, "Epsilon", kNormalPriority, 1,
        TaskDependency{4, "Delta"});

    EXPECT_EQ(
        registry.allTasks()[1].dependsOn,
        (std::optional<TaskDependency>{TaskDependency{4, "Delta"}}));
}

TEST(TaskRegistryTest, MarkStartedFlipsTheMatchingJobToRunning)
{
    TaskRegistry registry;
    registry.submit(1, "Alpha", kNormalPriority, 5);
    registry.submit(2, "Beta", kNormalPriority, 3);

    registry.markStarted(static_cast<JobId>(1));

    const auto tasks = registry.allTasks();
    EXPECT_EQ(tasks[0].status, TaskStatus::Running);
    EXPECT_EQ(tasks[1].status, TaskStatus::Pending);
}

TEST(TaskRegistryTest, MarkStartedIgnoresAJobIdNeverSubmitted)
{
    TaskRegistry registry;
    registry.submit(1, "Alpha", kNormalPriority, 5);

    registry.markStarted(static_cast<JobId>(99));

    EXPECT_EQ(registry.allTasks()[0].status, TaskStatus::Pending);
}

TEST(TaskRegistryTest, UpdateRemainingChangesTicksLeftForTheMatchingTaskId)
{
    TaskRegistry registry;
    registry.submit(7, "Render", kNormalPriority, 5);
    registry.markStarted(static_cast<JobId>(1));

    registry.updateRemaining(7, 3);

    EXPECT_EQ(registry.allTasks()[0].remainingTicks, 3U);
}

TEST(TaskRegistryTest, UpdateRemainingIgnoresATaskIdNeverSubmitted)
{
    TaskRegistry registry;
    registry.submit(7, "Render", kNormalPriority, 5);

    registry.updateRemaining(999, 3);

    EXPECT_EQ(registry.allTasks()[0].remainingTicks, 5U);
}

TEST(TaskRegistryTest, MarkCompletedFlipsStatusAndZeroesTicksLeft)
{
    TaskRegistry registry;
    registry.submit(7, "Render", kNormalPriority, 5);
    registry.markStarted(static_cast<JobId>(1));
    registry.updateRemaining(7, 2);

    registry.markCompleted(7);

    const auto &task = registry.allTasks()[0];
    EXPECT_EQ(task.status, TaskStatus::Completed);
    EXPECT_EQ(task.remainingTicks, 0U);
}

TEST(TaskRegistryTest, MarkCompletedIgnoresATaskIdNeverSubmitted)
{
    TaskRegistry registry;
    registry.submit(7, "Render", kNormalPriority, 5);

    registry.markCompleted(999);

    EXPECT_EQ(registry.allTasks()[0].status, TaskStatus::Pending);
}

TEST(TaskRegistryTest, TaskInfoEqualityComparesEveryFieldIndependently)
{
    const TaskInfo base{
        1, "Alpha", kNormalPriority, TaskStatus::Pending, 5, std::nullopt};

    EXPECT_NE(
        base,
        (TaskInfo{
            2, "Alpha", kNormalPriority, TaskStatus::Pending, 5,
            std::nullopt}));
    EXPECT_NE(
        base,
        (TaskInfo{
            1, "Beta", kNormalPriority, TaskStatus::Pending, 5,
            std::nullopt}));
    EXPECT_NE(
        base,
        (TaskInfo{
            1, "Alpha", kHighPriority, TaskStatus::Pending, 5,
            std::nullopt}));
    EXPECT_NE(
        base,
        (TaskInfo{
            1, "Alpha", kNormalPriority, TaskStatus::Running, 5,
            std::nullopt}));
    EXPECT_NE(
        base,
        (TaskInfo{
            1, "Alpha", kNormalPriority, TaskStatus::Pending, 6,
            std::nullopt}));
    EXPECT_NE(
        base,
        (TaskInfo{
            1, "Alpha", kNormalPriority, TaskStatus::Pending, 5,
            TaskDependency{4, "Delta"}}));
    EXPECT_EQ(
        base,
        (TaskInfo{
            1, "Alpha", kNormalPriority, TaskStatus::Pending, 5,
            std::nullopt}));
}

TEST(TaskRegistryTest, TaskDependencyEqualityComparesBothFields)
{
    const TaskDependency base{4, "Delta"};

    EXPECT_NE(base, (TaskDependency{5, "Delta"}));
    EXPECT_NE(base, (TaskDependency{4, "Epsilon"}));
    EXPECT_EQ(base, (TaskDependency{4, "Delta"}));
}
