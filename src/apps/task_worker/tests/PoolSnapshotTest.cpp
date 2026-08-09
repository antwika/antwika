#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

#include <antwika/ecs/World.hpp>
#include <antwika/ecs_commons/Name.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/scheduler/JobId.hpp>
#include <antwika/scheduler/Priority.hpp>

#include "antwika/task_worker/PoolSnapshot.hpp"
#include "antwika/task_worker/TaskRegistry.hpp"
#include "antwika/task_worker/Worker.hpp"

using antwika::ecs::World;
using antwika::ecs_commons::makeName;
using antwika::log::mocks::MockLogger;
using antwika::scheduler::JobId;
using antwika::scheduler::kCriticalPriority;
using antwika::scheduler::kHighPriority;
using antwika::scheduler::kLowPriority;
using antwika::scheduler::kNormalPriority;
using antwika::task_worker::DispatchInfo;
using antwika::task_worker::PoolSnapshot;
using antwika::task_worker::snapshotOf;
using antwika::task_worker::TaskDependency;
using antwika::task_worker::TaskRegistry;
using antwika::task_worker::TaskView;
using antwika::task_worker::Worker;
using antwika::task_worker::WorkerStatus;
using antwika::task_worker::WorkerView;
using ::testing::NiceMock;

namespace
{
    void addWorker(World &world, const Worker &worker)
    {
        const auto entity = world.create();
        world.add<Worker>(entity, worker);
    }

    [[nodiscard]] std::vector<std::string> labelsOf(
        const std::vector<TaskView> &tasks)
    {
        std::vector<std::string> labels;

        for (const auto &task : tasks)
        {
            labels.push_back(task.label);
        }

        return labels;
    }
}

TEST(PoolSnapshotTest, SnapshotOf_ReportsEveryWorkerInCreationOrder)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    addWorker(world, Worker{WorkerStatus::Busy, 3, 1, makeName("Alpha")});
    addWorker(world, Worker{});
    world.commit();

    TaskRegistry registry;
    registry.submit(1, "Alpha", kNormalPriority, 4);
    registry.markStarted(static_cast<JobId>(1));

    const auto snapshot = snapshotOf(world, registry, 7);

    EXPECT_EQ(snapshot.tick, 7U);
    EXPECT_EQ(
        snapshot.workers,
        (std::vector<WorkerView>{
            WorkerView{WorkerStatus::Busy, 1, "Alpha", 4, 3},
            WorkerView{}}));
}

TEST(PoolSnapshotTest, SnapshotOf_ReportsNoDurationForATaskItCannotFind)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    addWorker(world, Worker{WorkerStatus::Busy, 2, 99, makeName("Ghost")});
    world.commit();

    const TaskRegistry registry;

    const auto snapshot = snapshotOf(world, registry, 0);

    EXPECT_EQ(
        snapshot.workers,
        (std::vector<WorkerView>{
            WorkerView{WorkerStatus::Busy, 99, "Ghost", 0, 2}}));
}

TEST(PoolSnapshotTest, SnapshotOf_QueuesPendingTasksInSchedulerPullOrder)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    world.commit();

    TaskRegistry registry;
    registry.submit(1, "Alpha", kNormalPriority, 2);
    registry.submit(2, "Beta", kLowPriority, 2);
    registry.submit(3, "Gamma", kNormalPriority, 2);
    registry.submit(4, "Delta", kCriticalPriority, 2);
    registry.submit(5, "Epsilon", kHighPriority, 2);

    const auto snapshot = snapshotOf(world, registry, 0);

    EXPECT_EQ(
        labelsOf(snapshot.queue),
        (std::vector<std::string>{
            "Delta", "Epsilon", "Alpha", "Gamma", "Beta"}));
}

TEST(PoolSnapshotTest, SnapshotOf_PutsBlockedTasksAfterTheOnesInLine)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    world.commit();

    TaskRegistry registry;
    registry.submit(1, "Delta", kCriticalPriority, 1);
    registry.submit(
        2, "Epsilon", kCriticalPriority, 1, TaskDependency{1, "Delta"});
    registry.submit(3, "Gamma", kLowPriority, 1);

    const auto snapshot = snapshotOf(world, registry, 0);

    EXPECT_EQ(
        snapshot.queue,
        (std::vector<TaskView>{
            TaskView{1, "Delta", kCriticalPriority, 1, false, ""},
            TaskView{3, "Gamma", kLowPriority, 1, false, ""},
            TaskView{
                2, "Epsilon", kCriticalPriority, 1, true, "Delta"}}));
}

TEST(PoolSnapshotTest, SnapshotOf_UnblocksATaskOnceItsDependencyIsDone)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    world.commit();

    TaskRegistry registry;
    registry.submit(1, "Delta", kNormalPriority, 1);
    registry.submit(
        2, "Epsilon", kNormalPriority, 1, TaskDependency{1, "Delta"});
    registry.markStarted(static_cast<JobId>(1));
    registry.markCompleted(1);

    const auto snapshot = snapshotOf(world, registry, 0);

    ASSERT_EQ(snapshot.queue.size(), 1U);
    EXPECT_EQ(snapshot.queue[0].label, "Epsilon");
    EXPECT_FALSE(snapshot.queue[0].blocked);
}

TEST(PoolSnapshotTest, SnapshotOf_KeepsATaskBlockedOnADependencyItCannotFind)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    world.commit();

    TaskRegistry registry;
    registry.submit(
        2, "Epsilon", kNormalPriority, 1, TaskDependency{99, "Ghost"});

    const auto snapshot = snapshotOf(world, registry, 0);

    ASSERT_EQ(snapshot.queue.size(), 1U);
    EXPECT_TRUE(snapshot.queue[0].blocked);
    EXPECT_EQ(snapshot.queue[0].waitingFor, "Ghost");
}

TEST(PoolSnapshotTest, SnapshotOf_SeparatesRunningCompletedAndPendingTasks)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    world.commit();

    TaskRegistry registry;
    registry.submit(1, "Alpha", kNormalPriority, 2);
    registry.submit(2, "Beta", kNormalPriority, 2);
    registry.submit(3, "Gamma", kNormalPriority, 2);
    registry.markStarted(static_cast<JobId>(1));
    registry.markCompleted(1);
    registry.markStarted(static_cast<JobId>(2));

    const auto snapshot = snapshotOf(world, registry, 0);

    EXPECT_EQ(
        labelsOf(snapshot.completed), (std::vector<std::string>{"Alpha"}));
    EXPECT_EQ(
        labelsOf(snapshot.queue), (std::vector<std::string>{"Gamma"}));
}

TEST(PoolSnapshotTest, SnapshotOf_CarriesTheBudgetTheRegistryWasTold)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    world.commit();

    TaskRegistry registry;
    registry.noteDispatch(2, 1);

    EXPECT_EQ(
        snapshotOf(world, registry, 0).dispatch, (DispatchInfo{2, 1}));
}

TEST(PoolSnapshotTest, OperatorEquals_ComparesEveryViewField)
{
    const WorkerView worker{WorkerStatus::Busy, 1, "Alpha", 4, 3};
    EXPECT_NE(worker, (WorkerView{WorkerStatus::Idle, 1, "Alpha", 4, 3}));
    EXPECT_NE(worker, (WorkerView{WorkerStatus::Busy, 2, "Alpha", 4, 3}));
    EXPECT_NE(worker, (WorkerView{WorkerStatus::Busy, 1, "Beta", 4, 3}));
    EXPECT_NE(worker, (WorkerView{WorkerStatus::Busy, 1, "Alpha", 5, 3}));
    EXPECT_NE(worker, (WorkerView{WorkerStatus::Busy, 1, "Alpha", 4, 2}));
    EXPECT_EQ(worker, (WorkerView{WorkerStatus::Busy, 1, "Alpha", 4, 3}));

    const TaskView task{1, "Alpha", kNormalPriority, 2, false, ""};
    EXPECT_NE(task, (TaskView{2, "Alpha", kNormalPriority, 2, false, ""}));
    EXPECT_NE(task, (TaskView{1, "Beta", kNormalPriority, 2, false, ""}));
    EXPECT_NE(task, (TaskView{1, "Alpha", kLowPriority, 2, false, ""}));
    EXPECT_NE(task, (TaskView{1, "Alpha", kNormalPriority, 3, false, ""}));
    EXPECT_NE(task, (TaskView{1, "Alpha", kNormalPriority, 2, true, ""}));
    EXPECT_NE(task, (TaskView{1, "Alpha", kNormalPriority, 2, false, "x"}));
    EXPECT_EQ(task, (TaskView{1, "Alpha", kNormalPriority, 2, false, ""}));

    PoolSnapshot snapshot;
    snapshot.tick = 1;
    snapshot.dispatch = DispatchInfo{2, 1};
    snapshot.workers = {WorkerView{}};
    snapshot.queue = {task};
    snapshot.completed = {task};

    PoolSnapshot same = snapshot;
    EXPECT_EQ(snapshot, same);

    PoolSnapshot other = snapshot;
    other.tick = 2;
    EXPECT_NE(snapshot, other);

    other = snapshot;
    other.dispatch = DispatchInfo{2, 2};
    EXPECT_NE(snapshot, other);

    other = snapshot;
    other.workers.clear();
    EXPECT_NE(snapshot, other);

    other = snapshot;
    other.queue.clear();
    EXPECT_NE(snapshot, other);

    other = snapshot;
    other.completed.clear();
    EXPECT_NE(snapshot, other);
}
