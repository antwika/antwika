#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <string>
#include <vector>

#include <antwika/scheduler/fakes/FakeRecordingJob.hpp>
#include <antwika/scheduler/fakes/FakeReschedulingJob.hpp>
#include <antwika/scheduler/mocks/MockJob.hpp>

#include "antwika/scheduler/Scheduler.hpp"
#include "antwika/scheduler/JobId.hpp"
#include "antwika/scheduler/Priority.hpp"
#include "antwika/scheduler/SchedulerError.hpp"

using antwika::scheduler::JobId;
using antwika::scheduler::kHighPriority;
using antwika::scheduler::kInvalidJobId;
using antwika::scheduler::kLowPriority;
using antwika::scheduler::kNormalPriority;
using antwika::scheduler::rawValue;
using antwika::scheduler::Scheduler;
using antwika::scheduler::SchedulerError;
using antwika::scheduler::mocks::MockJob;
using antwika::scheduler::fakes::FakeRecordingJob;
using antwika::scheduler::fakes::FakeReschedulingJob;
using ::testing::NiceMock;

namespace
{

}

TEST(SchedulerDependencyTest, Run_WaitsForAnUnmetDependency)
{
    Scheduler scheduler;
    std::vector<std::string> log;
    FakeRecordingJob first(log, "first");
    FakeRecordingJob second(log, "second");

    const auto firstId = scheduler.schedule(first, kNormalPriority);
    scheduler.schedule(second, kNormalPriority, {firstId});

    const auto beforeRun = scheduler.run(0, 1);
    EXPECT_EQ(beforeRun, (std::vector<JobId>{firstId}));
    EXPECT_EQ(log, (std::vector<std::string>{"first"}));

    const auto afterRun = scheduler.run(0, 1);
    EXPECT_EQ(afterRun.size(), 1U);
    EXPECT_EQ(log, (std::vector<std::string>{"first", "second"}));
}

TEST(SchedulerDependencyTest, Run_ReadiesAJobDependingOnACompletedOne)
{
    Scheduler scheduler;
    std::vector<std::string> log;
    FakeRecordingJob first(log, "first");
    FakeRecordingJob second(log, "second");

    const auto firstId = scheduler.schedule(first, kNormalPriority);
    scheduler.run(0, 1);
    EXPECT_EQ(log, (std::vector<std::string>{"first"}));

    scheduler.schedule(second, kNormalPriority, {firstId});
    const auto ran = scheduler.run(0, 1);
    EXPECT_EQ(ran.size(), 1U);
    EXPECT_EQ(log, (std::vector<std::string>{"first", "second"}));
}

TEST(SchedulerDependencyTest, Run_ResolvesADiamondOnceBothParentsComplete)
{
    Scheduler scheduler;
    std::vector<std::string> log;
    FakeRecordingJob a(log, "A");
    FakeRecordingJob b(log, "B");
    FakeRecordingJob c(log, "C");
    FakeRecordingJob d(log, "D");

    const auto idA = scheduler.schedule(a, kNormalPriority);
    const auto idB = scheduler.schedule(b, kLowPriority, {idA});
    const auto idC = scheduler.schedule(c, kHighPriority, {idA});
    scheduler.schedule(d, kNormalPriority, {idB, idC});

    EXPECT_EQ(scheduler.pending(), 4U);

    const auto executed = scheduler.run(0, 10);

    EXPECT_EQ(executed.size(), 4U);
    EXPECT_EQ(log, (std::vector<std::string>{"A", "C", "B", "D"}));
}

TEST(SchedulerDependencyTest, Run_CascadesAPreExistingChainInOneCall)
{
    Scheduler scheduler;
    std::vector<std::string> log;
    FakeRecordingJob a(log, "A");
    FakeRecordingJob b(log, "B");
    FakeRecordingJob c(log, "C");

    const auto idA = scheduler.schedule(a, kNormalPriority);
    const auto idB = scheduler.schedule(b, kNormalPriority, {idA});
    scheduler.schedule(c, kNormalPriority, {idB});

    const auto executed = scheduler.run(0, 10);

    EXPECT_EQ(executed.size(), 3U);
    EXPECT_EQ(log, (std::vector<std::string>{"A", "B", "C"}));
    EXPECT_TRUE(scheduler.empty());
}

TEST(SchedulerDependencyTest, Run_HoldsTheEpochRuleForNewDependents)
{
    Scheduler scheduler;
    std::vector<std::string> log;
    FakeRecordingJob a(log, "A");
    FakeRecordingJob next(log, "next");

    const auto idA = scheduler.schedule(a, kNormalPriority);
    FakeReschedulingJob rescheduler(scheduler, next, {idA});
    scheduler.schedule(rescheduler, kHighPriority);

    const auto firstRun = scheduler.run(0, 10);

    EXPECT_EQ(firstRun.size(), 2U);
    EXPECT_EQ(log, (std::vector<std::string>{"A"}));
    EXPECT_EQ(scheduler.pending(), 1U);

    const auto secondRun = scheduler.run(0, 10);

    EXPECT_EQ(secondRun.size(), 1U);
    EXPECT_EQ(log, (std::vector<std::string>{"A", "next"}));
}

TEST(SchedulerDependencyTest, Schedule_ThrowsOnAnUnknownDependencyId)
{
    Scheduler scheduler;
    NiceMock<MockJob> job;

    EXPECT_THROW(
        scheduler.schedule(job, kNormalPriority, {kInvalidJobId}),
        SchedulerError);
    EXPECT_TRUE(scheduler.empty());

    NiceMock<MockJob> other;
    const auto otherId = scheduler.schedule(other, kNormalPriority);
    const auto foreignId = static_cast<JobId>(rawValue(otherId) + 100);

    EXPECT_THROW(
        scheduler.schedule(job, kNormalPriority, {foreignId}),
        SchedulerError);
    EXPECT_EQ(scheduler.pending(), 1U);
}

TEST(SchedulerDependencyTest, Schedule_ThrowsOnADependencyOnTheIdItWillGet)
{
    Scheduler scheduler;
    NiceMock<MockJob> first;
    NiceMock<MockJob> second;

    const auto firstId = scheduler.schedule(first, kNormalPriority);
    const auto ownId = static_cast<JobId>(rawValue(firstId) + 1);

    EXPECT_THROW(
        scheduler.schedule(second, kNormalPriority, {ownId}),
        SchedulerError);
    EXPECT_EQ(scheduler.pending(), 1U);
}

TEST(SchedulerDependencyTest, Schedule_TreatsADuplicateDependencyAsOne)
{
    Scheduler scheduler;
    std::vector<std::string> log;
    FakeRecordingJob first(log, "first");
    FakeRecordingJob second(log, "second");

    const auto firstId = scheduler.schedule(first, kNormalPriority);
    scheduler.schedule(second, kNormalPriority, {firstId, firstId});

    const auto executed = scheduler.run(0, 10);

    EXPECT_EQ(executed.size(), 2U);
    EXPECT_EQ(log, (std::vector<std::string>{"first", "second"}));
}

TEST(SchedulerDependencyTest, Run_LeavesADependentPendingWhileUnresolved)
{
    Scheduler scheduler;
    NiceMock<MockJob> starvedLow;
    EXPECT_CALL(starvedLow, execute(::testing::_)).Times(0);
    NiceMock<MockJob> dependent;
    EXPECT_CALL(dependent, execute(::testing::_)).Times(0);

    const auto lowId = scheduler.schedule(starvedLow, kLowPriority);
    scheduler.schedule(dependent, kNormalPriority, {lowId});

    for (int i = 0; i < 5; ++i)
    {
        NiceMock<MockJob> critical;
        EXPECT_CALL(critical, execute(::testing::_)).Times(1);
        scheduler.schedule(critical, kHighPriority);
        scheduler.run(0, 1);
    }

    EXPECT_EQ(scheduler.pending(), 2U);
}
