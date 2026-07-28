#include "antwika/scheduler/Scheduler.hpp"

#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/scheduler/mocks/MockJob.hpp>

#include "antwika/scheduler/JobId.hpp"
#include "antwika/scheduler/Priority.hpp"

using antwika::scheduler::JobId;
using antwika::scheduler::kCriticalPriority;
using antwika::scheduler::kHighPriority;
using antwika::scheduler::kLowPriority;
using antwika::scheduler::kNormalPriority;
using antwika::scheduler::rawValue;
using antwika::scheduler::Scheduler;
using antwika::scheduler::mocks::MockJob;
using ::testing::NiceMock;
using ::testing::Return;

namespace
{

    class RecordingJob final : public antwika::scheduler::IJob
    {
    public:
        explicit RecordingJob(std::vector<std::string> &log, std::string name)
            : log(log), name(std::move(name))
        {
        }

        void execute(antwika::time::Tick) override
        {
            log.push_back(name);
        }

    private:
        std::vector<std::string> &log;
        std::string name;
    };

    class ReschedulingJob final : public antwika::scheduler::IJob
    {
    public:
        ReschedulingJob(Scheduler &scheduler, antwika::scheduler::IJob &next)
            : scheduler(scheduler), next(next)
        {
        }

        void execute(antwika::time::Tick) override
        {
            scheduler.schedule(next, kNormalPriority);
        }

    private:
        Scheduler &scheduler;
        antwika::scheduler::IJob &next;
    };

} // namespace

TEST(SchedulerTest, ScheduleReturnsStrictlyIncreasingJobIdsStartingAtOne)
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

TEST(SchedulerTest, PendingAndEmptyTrackQueueSizeThroughScheduleRunCycles)
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

TEST(SchedulerTest, PendingCountsBlockedJobsToo)
{
    Scheduler scheduler;
    NiceMock<MockJob> jobA;
    NiceMock<MockJob> jobB;

    const auto idA = scheduler.schedule(jobA, kNormalPriority);
    scheduler.schedule(jobB, kNormalPriority, {idA});

    EXPECT_EQ(scheduler.pending(), 2U);
}

TEST(SchedulerTest, RunExecutesHighestPriorityFirstThenFifoWithinPriority)
{
    Scheduler scheduler;
    std::vector<std::string> log;
    RecordingJob low(log, "low");
    RecordingJob normalFirst(log, "normalFirst");
    RecordingJob normalSecond(log, "normalSecond");
    RecordingJob high(log, "high");

    // Deliberately scheduled out of priority order.
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

TEST(SchedulerTest, RunNeverExecutesMoreThanBudget)
{
    Scheduler scheduler;
    NiceMock<MockJob> jobA;
    NiceMock<MockJob> jobB;
    NiceMock<MockJob> jobC;
    EXPECT_CALL(jobA, execute(::testing::_)).Times(1);
    EXPECT_CALL(jobB, execute(::testing::_)).Times(1);
    EXPECT_CALL(jobC, execute(::testing::_)).Times(0);

    scheduler.schedule(jobA, kNormalPriority);
    scheduler.schedule(jobB, kNormalPriority);
    scheduler.schedule(jobC, kNormalPriority);

    const auto executed = scheduler.run(0, 2);

    EXPECT_EQ(executed.size(), 2U);
    EXPECT_EQ(scheduler.pending(), 1U);
}

TEST(SchedulerTest, JobScheduledDuringExecuteIsExcludedFromThatRunCall)
{
    Scheduler scheduler;
    std::vector<std::string> log;
    RecordingJob next(log, "next");
    ReschedulingJob first(scheduler, next);

    scheduler.schedule(first, kNormalPriority);

    const auto firstRun = scheduler.run(0, 10);

    EXPECT_EQ(firstRun.size(), 1U);
    EXPECT_TRUE(log.empty());
    EXPECT_EQ(scheduler.pending(), 1U);

    const auto secondRun = scheduler.run(0, 10);

    EXPECT_EQ(secondRun.size(), 1U);
    EXPECT_EQ(log, (std::vector<std::string>{"next"}));
}

TEST(SchedulerTest, BudgetZeroIsATrueNoOp)
{
    Scheduler scheduler;
    NiceMock<MockJob> job;
    EXPECT_CALL(job, execute(::testing::_)).Times(0);

    scheduler.schedule(job, kNormalPriority);

    const auto executed = scheduler.run(0, 0);

    EXPECT_TRUE(executed.empty());
    EXPECT_EQ(scheduler.pending(), 1U);
}

TEST(SchedulerTest, LowPriorityJobCanStarveIndefinitely)
{
    Scheduler scheduler;
    NiceMock<MockJob> lowJob;
    EXPECT_CALL(lowJob, execute(::testing::_)).Times(0);
    scheduler.schedule(lowJob, kLowPriority);

    for (int i = 0; i < 5; ++i)
    {
        NiceMock<MockJob> criticalJob;
        EXPECT_CALL(criticalJob, execute(::testing::_)).Times(1);
        scheduler.schedule(criticalJob, kCriticalPriority);
        scheduler.run(0, 1);
    }

    EXPECT_EQ(scheduler.pending(), 1U);
}
