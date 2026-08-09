#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <antwika/time/Tick.hpp>

#include <antwika/scheduler/fakes/FakeDestructionTrackingJob.hpp>
#include <antwika/scheduler/fakes/FakeRecordingJob.hpp>
#include <antwika/scheduler/fakes/FakeReschedulingJob.hpp>
#include <antwika/scheduler/mocks/MockJob.hpp>

#include "antwika/scheduler/Scheduler.hpp"
#include "antwika/scheduler/JobId.hpp"
#include "antwika/scheduler/Priority.hpp"
#include "antwika/scheduler/SchedulerError.hpp"

using antwika::scheduler::JobId;
using antwika::scheduler::kInvalidJobId;
using antwika::scheduler::kCriticalPriority;
using antwika::scheduler::kHighPriority;
using antwika::scheduler::kLowPriority;
using antwika::scheduler::kNormalPriority;
using antwika::scheduler::rawValue;
using antwika::scheduler::Scheduler;
using antwika::scheduler::SchedulerError;
using antwika::scheduler::mocks::MockJob;
using antwika::scheduler::fakes::FakeDestructionTrackingJob;
using antwika::scheduler::fakes::FakeRecordingJob;
using antwika::scheduler::fakes::FakeReschedulingJob;
using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::Throw;

namespace
{

}

TEST(SchedulerTest, Schedule_ReturnsIncreasingIdsFromOne)
{
    Scheduler scheduler;
    NiceMock<MockJob> jobA;
    NiceMock<MockJob> jobB;
    NiceMock<MockJob> jobC;

    const auto idA = scheduler.schedule(jobA, kNormalPriority);
    const auto idB = scheduler.schedule(jobB, kNormalPriority);
    const auto idC = scheduler.schedule(jobC, kNormalPriority);

    EXPECT_EQ(rawValue(idA), 1U);
    EXPECT_EQ(rawValue(idB), 2U);
    EXPECT_EQ(rawValue(idC), 3U);
}

TEST(SchedulerTest, KInvalidJobId_IsAnIdNoScheduledJobIsGiven)
{
    Scheduler scheduler;
    NiceMock<MockJob> job;

    EXPECT_EQ(rawValue(kInvalidJobId), 0U);
    EXPECT_NE(scheduler.schedule(job, kNormalPriority), kInvalidJobId);
}

TEST(SchedulerTest, Pending_TracksTheQueueThroughRunCycles)
{
    Scheduler scheduler;
    NiceMock<MockJob> jobA;
    NiceMock<MockJob> jobB;

    EXPECT_TRUE(scheduler.empty());
    EXPECT_EQ(scheduler.pending(), 0U);

    scheduler.schedule(jobA, kNormalPriority);
    scheduler.schedule(jobB, kNormalPriority);

    EXPECT_FALSE(scheduler.empty());
    EXPECT_EQ(scheduler.pending(), 2U);

    scheduler.run(0, 1);

    EXPECT_EQ(scheduler.pending(), 1U);

    scheduler.run(0, 1);

    EXPECT_TRUE(scheduler.empty());
}

TEST(SchedulerTest, Pending_CountsBlockedJobsToo)
{
    Scheduler scheduler;
    NiceMock<MockJob> jobA;
    NiceMock<MockJob> jobB;

    const auto idA = scheduler.schedule(jobA, kNormalPriority);
    scheduler.schedule(jobB, kNormalPriority, {idA});

    EXPECT_EQ(scheduler.pending(), 2U);
}

TEST(SchedulerTest, Run_TakesHighestPriorityThenFifo)
{
    Scheduler scheduler;
    std::vector<std::string> log;
    FakeRecordingJob low(log, "low");
    FakeRecordingJob normalFirst(log, "normalFirst");
    FakeRecordingJob normalSecond(log, "normalSecond");
    FakeRecordingJob high(log, "high");

    scheduler.schedule(low, kLowPriority);
    scheduler.schedule(normalFirst, kNormalPriority);
    scheduler.schedule(high, kHighPriority);
    scheduler.schedule(normalSecond, kNormalPriority);

    const auto executed = scheduler.run(0, 10);

    EXPECT_EQ(executed.size(), 4U);
    EXPECT_EQ(
        log,
        (std::vector<std::string>{
            "high", "normalFirst", "normalSecond", "low"}));
}

TEST(SchedulerTest, Run_OrdersTheFourPrioritiesFromCriticalDownToLow)
{
    Scheduler scheduler;
    std::vector<std::string> log;
    FakeRecordingJob low(log, "low");
    FakeRecordingJob normal(log, "normal");
    FakeRecordingJob high(log, "high");
    FakeRecordingJob critical(log, "critical");

    scheduler.schedule(low, kLowPriority);
    scheduler.schedule(normal, kNormalPriority);
    scheduler.schedule(high, kHighPriority);
    scheduler.schedule(critical, kCriticalPriority);

    const auto executed = scheduler.run(0, 10);

    EXPECT_EQ(executed.size(), 4U);
    EXPECT_EQ(
        log,
        (std::vector<std::string>{"critical", "high", "normal", "low"}));
}

TEST(SchedulerTest, Run_NeverExecutesMoreThanTheBudget)
{
    Scheduler scheduler;
    NiceMock<MockJob> jobA;
    NiceMock<MockJob> jobB;
    NiceMock<MockJob> jobC;
    EXPECT_CALL(jobA, execute(_)).Times(1);
    EXPECT_CALL(jobB, execute(_)).Times(1);
    EXPECT_CALL(jobC, execute(_)).Times(0);

    scheduler.schedule(jobA, kNormalPriority);
    scheduler.schedule(jobB, kNormalPriority);
    scheduler.schedule(jobC, kNormalPriority);

    const auto executed = scheduler.run(0, 2);

    EXPECT_EQ(executed.size(), 2U);
    EXPECT_EQ(scheduler.pending(), 1U);
}

TEST(SchedulerTest, Run_HandsEachJobTheTickItWasGiven)
{
    Scheduler scheduler;
    NiceMock<MockJob> jobA;
    NiceMock<MockJob> jobB;
    EXPECT_CALL(jobA, execute(antwika::time::Tick{42}));
    EXPECT_CALL(jobB, execute(antwika::time::Tick{42}));

    scheduler.schedule(jobA, kNormalPriority);
    scheduler.schedule(jobB, kNormalPriority);

    EXPECT_EQ(scheduler.run(42, 10).size(), 2U);
}

TEST(SchedulerTest, Run_ExcludesAJobScheduledDuringIt)
{
    Scheduler scheduler;
    std::vector<std::string> log;
    FakeRecordingJob next(log, "next");
    FakeReschedulingJob first(scheduler, next);

    scheduler.schedule(first, kNormalPriority);

    const auto firstRun = scheduler.run(0, 10);

    EXPECT_EQ(firstRun.size(), 1U);
    EXPECT_TRUE(log.empty());
    EXPECT_EQ(scheduler.pending(), 1U);

    const auto secondRun = scheduler.run(0, 10);

    EXPECT_EQ(secondRun.size(), 1U);
    EXPECT_EQ(log, (std::vector<std::string>{"next"}));
}

TEST(SchedulerTest, Run_DoesNothingWithAZeroBudget)
{
    Scheduler scheduler;
    NiceMock<MockJob> job;
    EXPECT_CALL(job, execute(_)).Times(0);

    scheduler.schedule(job, kNormalPriority);

    const auto executed = scheduler.run(0, 0);

    EXPECT_TRUE(executed.empty());
    EXPECT_EQ(scheduler.pending(), 1U);
}

TEST(SchedulerTest, Run_CanStarveALowPriorityJob)
{
    Scheduler scheduler;
    NiceMock<MockJob> lowJob;
    EXPECT_CALL(lowJob, execute(_)).Times(0);
    scheduler.schedule(lowJob, kLowPriority);

    for (int i = 0; i < 5; ++i)
    {
        auto criticalJob = std::make_unique<NiceMock<MockJob>>();
        EXPECT_CALL(*criticalJob, execute(_)).Times(1);
        scheduler.schedule(std::move(criticalJob), kCriticalPriority);
        scheduler.run(0, 1);
    }

    EXPECT_EQ(scheduler.pending(), 1U);
}

TEST(SchedulerTest, ScheduleOwned_RunsTheJobLikeAnyOther)
{
    Scheduler scheduler;
    std::vector<std::string> log;

    const auto id = scheduler.schedule(
        std::make_unique<FakeRecordingJob>(log, "owned"), kNormalPriority);
    const auto executed = scheduler.run(0, 10);

    EXPECT_EQ(executed, (std::vector<JobId>{id}));
    EXPECT_EQ(log, (std::vector<std::string>{"owned"}));
}

TEST(SchedulerTest, ScheduleOwned_ReleasesTheJobOnceItHasRun)
{
    Scheduler scheduler;
    bool destroyed = false;

    scheduler.schedule(
        std::make_unique<FakeDestructionTrackingJob>(destroyed),
        kNormalPriority);

    EXPECT_FALSE(destroyed);

    scheduler.run(0, 10);

    EXPECT_TRUE(destroyed);
}

TEST(SchedulerTest, ScheduleOwned_KeepsAnUnrunJobUntilItDies)
{
    bool destroyed = false;

    {
        Scheduler scheduler;
        scheduler.schedule(
            std::make_unique<FakeDestructionTrackingJob>(destroyed),
            kNormalPriority);

        EXPECT_FALSE(destroyed);
    }

    EXPECT_TRUE(destroyed);
}

TEST(SchedulerTest, ScheduleOwned_InterleavesWithBorrowedJobs)
{
    Scheduler scheduler;
    std::vector<std::string> log;
    FakeRecordingJob borrowed(log, "borrowed");
    bool destroyed = false;

    const auto borrowedId = scheduler.schedule(borrowed, kNormalPriority);
    const auto ownedId = scheduler.schedule(
        std::make_unique<FakeDestructionTrackingJob>(destroyed),
        kNormalPriority);

    const auto executed = scheduler.run(0, 10);

    EXPECT_EQ(executed, (std::vector<JobId>{borrowedId, ownedId}));
    EXPECT_EQ(log, (std::vector<std::string>{"borrowed"}));
    EXPECT_TRUE(destroyed);
}

TEST(SchedulerTest, Run_CountsAThrowingJobAsHavingRun)
{
    Scheduler scheduler;
    NiceMock<MockJob> failing;
    NiceMock<MockJob> waiting;

    EXPECT_CALL(failing, execute(_))
        .WillOnce(Throw(std::runtime_error("job failed")));

    const auto failingId = scheduler.schedule(failing, kNormalPriority);
    scheduler.schedule(waiting, kNormalPriority, {failingId});

    EXPECT_THROW(
        static_cast<void>(scheduler.run(0, 10)), std::runtime_error);

    EXPECT_EQ(scheduler.pending(), 1U);

    EXPECT_CALL(waiting, execute(_));
    const auto executed = scheduler.run(1, 10);

    EXPECT_EQ(executed.size(), 1U);
    EXPECT_TRUE(scheduler.empty());
}

TEST(SchedulerTest, ScheduleOwned_ThrowsOnANullJob)
{
    Scheduler scheduler;

    EXPECT_THROW(
        scheduler.schedule(
            std::unique_ptr<antwika::scheduler::IJob>{}, kNormalPriority),
        SchedulerError);
    EXPECT_EQ(scheduler.pending(), 0U);
}

TEST(SchedulerTest, ScheduleOwned_ReleasesTheJobWhenDependsOnFails)
{
    Scheduler scheduler;
    bool destroyed = false;

    EXPECT_THROW(
        scheduler.schedule(
            std::make_unique<FakeDestructionTrackingJob>(destroyed),
            kNormalPriority,
            {kInvalidJobId}),
        SchedulerError);

    EXPECT_TRUE(destroyed);
    EXPECT_EQ(scheduler.pending(), 0U);

    NiceMock<MockJob> job;
    EXPECT_EQ(rawValue(scheduler.schedule(job, kNormalPriority)), 1U);
}
