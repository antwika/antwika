#include "antwika/scheduler/Scheduler.hpp"

#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "antwika/scheduler/JobId.hpp"
#include "antwika/scheduler/Priority.hpp"

using antwika::scheduler::JobId;
using antwika::scheduler::kHighPriority;
using antwika::scheduler::kLowPriority;
using antwika::scheduler::kNormalPriority;
using antwika::scheduler::Scheduler;

namespace
{

    class NoOpJob final : public antwika::scheduler::IJob
    {
    public:
        void execute(antwika::time::Tick) override
        {
        }
    };

    std::vector<std::vector<JobId>> runOnce()
    {
        Scheduler scheduler;
        NoOpJob a;
        NoOpJob b;
        NoOpJob c;
        NoOpJob d;
        NoOpJob e;

        const auto idA = scheduler.schedule(a, kLowPriority);
        const auto idB = scheduler.schedule(b, kHighPriority);
        const auto idC = scheduler.schedule(c, kNormalPriority, {idA});
        scheduler.schedule(d, kNormalPriority);
        scheduler.schedule(e, kHighPriority, {idB, idC});

        std::vector<std::vector<JobId>> results;
        results.push_back(scheduler.run(0, 2));
        results.push_back(scheduler.run(1, 1));
        results.push_back(scheduler.run(2, 2));
        results.push_back(scheduler.run(3, 5));

        return results;
    }

} // namespace

TEST(SchedulerDeterminismTest, SameInputsProduceIdenticalRunSequences)
{
    const auto first = runOnce();
    const auto second = runOnce();

    EXPECT_EQ(first, second);
}
