#include "antwika/scheduler/Scheduler.hpp"

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/scheduler/mocks/MockJob.hpp>

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
using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::Throw;

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

    // Flips a caller-owned flag on destruction.
    // Lets a test pin down when the Scheduler releases an owned job.
    class DestructionTrackingJob final : public antwika::scheduler::IJob
    {
    public:
        explicit DestructionTrackingJob(bool &destroyed)
            : destroyed(destroyed)
        {
        }

        ~DestructionTrackingJob() override
        {
            destroyed = true;
        }

        DestructionTrackingJob(const DestructionTrackingJob &) = delete;
        DestructionTrackingJob(DestructionTrackingJob &&) = delete;

        DestructionTrackingJob &operator=(
            const DestructionTrackingJob &) = delete;
        DestructionTrackingJob &operator=(
            DestructionTrackingJob &&) = delete;

        void execute(antwika::time::Tick) override
        {
        }

    private:
        bool &destroyed;
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
    EXPECT_CALL(job, execute(_)).Times(0);

    scheduler.schedule(job, kNormalPriority);

    const auto executed = scheduler.run(0, 0);

    EXPECT_TRUE(executed.empty());
    EXPECT_EQ(scheduler.pending(), 1U);
}

TEST(SchedulerTest, LowPriorityJobCanStarveIndefinitely)
{
    Scheduler scheduler;
    NiceMock<MockJob> lowJob;
    EXPECT_CALL(lowJob, execute(_)).Times(0);
    scheduler.schedule(lowJob, kLowPriority);

    for (int i = 0; i < 5; ++i)
    {
        // Handed over, not kept on the loop's stack frame.
        // A per-iteration local would in fact run before it died.
        // But that leans on run() being the only dereference.
        // This follows the documented contract instead.
        auto criticalJob = std::make_unique<NiceMock<MockJob>>();
        EXPECT_CALL(*criticalJob, execute(_)).Times(1);
        scheduler.schedule(std::move(criticalJob), kCriticalPriority);
        scheduler.run(0, 1);
    }

    EXPECT_EQ(scheduler.pending(), 1U);
}

TEST(SchedulerTest, ScheduleTakingOwnershipRunsTheJobLikeAnyOther)
{
    Scheduler scheduler;
    std::vector<std::string> log;

    const auto id = scheduler.schedule(
        std::make_unique<RecordingJob>(log, "owned"), kNormalPriority);
    const auto executed = scheduler.run(0, 10);

    EXPECT_EQ(executed, (std::vector<JobId>{id}));
    EXPECT_EQ(log, (std::vector<std::string>{"owned"}));
}

TEST(SchedulerTest, ScheduleTakingOwnershipReleasesTheJobOnceItHasRun)
{
    Scheduler scheduler;
    bool destroyed = false;

    scheduler.schedule(
        std::make_unique<DestructionTrackingJob>(destroyed),
        kNormalPriority);

    EXPECT_FALSE(destroyed);

    scheduler.run(0, 10);

    // A long session submits one job per event and never stops.
    // So a job outliving its own run is memory nothing will free.
    EXPECT_TRUE(destroyed);
}

TEST(SchedulerTest, ScheduleTakingOwnershipKeepsAnUnrunJobUntilItDies)
{
    bool destroyed = false;

    {
        Scheduler scheduler;
        scheduler.schedule(
            std::make_unique<DestructionTrackingJob>(destroyed),
            kNormalPriority);

        EXPECT_FALSE(destroyed);
    }

    EXPECT_TRUE(destroyed);
}

TEST(SchedulerTest, ScheduleTakingOwnershipInterleavesWithBorrowedJobs)
{
    // Owned and borrowed jobs share one JobId space.
    // A releasing Scheduler has to free the right slot for either.
    Scheduler scheduler;
    std::vector<std::string> log;
    RecordingJob borrowed(log, "borrowed");
    bool destroyed = false;

    const auto borrowedId = scheduler.schedule(borrowed, kNormalPriority);
    const auto ownedId = scheduler.schedule(
        std::make_unique<DestructionTrackingJob>(destroyed),
        kNormalPriority);

    const auto executed = scheduler.run(0, 10);

    EXPECT_EQ(executed, (std::vector<JobId>{borrowedId, ownedId}));
    EXPECT_EQ(log, (std::vector<std::string>{"borrowed"}));
    EXPECT_TRUE(destroyed);
}

TEST(SchedulerTest, AJobThatThrowsStillCountsAsHavingRun)
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

    // Left half-run it would be gone from ready.
    // It would still be counted as pending.
    // Its dependent would wait on it forever.
    EXPECT_EQ(scheduler.pending(), 1U);

    EXPECT_CALL(waiting, execute(_));
    const auto executed = scheduler.run(1, 10);

    EXPECT_EQ(executed.size(), 1U);
    EXPECT_TRUE(scheduler.empty());
}

TEST(SchedulerTest, ScheduleTakingOwnershipOfANullJobThrows)
{
    Scheduler scheduler;

    EXPECT_THROW(
        scheduler.schedule(
            std::unique_ptr<antwika::scheduler::IJob>{}, kNormalPriority),
        SchedulerError);
    EXPECT_EQ(scheduler.pending(), 0U);
}

TEST(SchedulerTest, ScheduleTakingOwnershipReleasesTheJobWhenDependsOnFails)
{
    Scheduler scheduler;
    bool destroyed = false;

    EXPECT_THROW(
        scheduler.schedule(
            std::make_unique<DestructionTrackingJob>(destroyed),
            kNormalPriority,
            {kInvalidJobId}),
        SchedulerError);

    EXPECT_TRUE(destroyed);
    EXPECT_EQ(scheduler.pending(), 0U);

    // Nothing was mutated, so the next job still gets JobId 1.
    NiceMock<MockJob> job;
    EXPECT_EQ(rawValue(scheduler.schedule(job, kNormalPriority)), 1U);
}
