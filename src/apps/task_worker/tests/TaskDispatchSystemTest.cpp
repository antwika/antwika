#include "antwika/task_worker/TaskDispatchSystem.hpp"

#include <gtest/gtest.h>

#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/scheduler/mocks/MockJob.hpp>

#include "antwika/task_worker/JobQueue.hpp"
#include "antwika/task_worker/TaskRegistry.hpp"
#include "antwika/task_worker/Worker.hpp"
#include "antwika/task_worker/WorkerLookup.hpp"

using antwika::ecs::World;
using antwika::log::mocks::MockLogger;
using antwika::scheduler::kNormalPriority;
using antwika::scheduler::mocks::MockJob;
using antwika::task_worker::JobQueue;
using antwika::task_worker::TaskDispatchSystem;
using antwika::task_worker::TaskRegistry;
using antwika::task_worker::Worker;
using antwika::task_worker::WorkerLookup;
using antwika::task_worker::WorkerStatus;
using ::testing::NiceMock;

TEST(TaskDispatchSystemTest, RunsSchedulerWithExactlyTheIdleWorkerCount)
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
    JobQueue jobs;
    NiceMock<MockJob> jobA;
    NiceMock<MockJob> jobB;
    NiceMock<MockJob> jobC;
    EXPECT_CALL(jobA, execute(::testing::_)).Times(1);
    EXPECT_CALL(jobB, execute(::testing::_)).Times(1);
    EXPECT_CALL(jobC, execute(::testing::_)).Times(0);
    jobs.scheduler().schedule(jobA, kNormalPriority);
    jobs.scheduler().schedule(jobB, kNormalPriority);
    jobs.scheduler().schedule(jobC, kNormalPriority);

    TaskRegistry registry;
    TaskDispatchSystem system(jobs, lookup, registry);
    system.update(world, 0);

    EXPECT_EQ(jobs.scheduler().pending(), 1U);
}
