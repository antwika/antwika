#pragma once

#include <utility>
#include <vector>

#include <antwika/time/Tick.hpp>

#include "antwika/scheduler/IJob.hpp"
#include "antwika/scheduler/JobId.hpp"
#include "antwika/scheduler/Priority.hpp"
#include "antwika/scheduler/Scheduler.hpp"

namespace antwika::scheduler::fakes
{

    class FakeReschedulingJob final : public IJob
    {
    public:
        FakeReschedulingJob(
            Scheduler &scheduler,
            IJob &next,
            std::vector<JobId> dependsOn = {})
            : scheduler(scheduler),
              next(next),
              dependsOn(std::move(dependsOn))
        {
        }

        void execute(antwika::time::Tick) override
        {
            scheduler.schedule(next, kNormalPriority, dependsOn);
        }

    private:
        Scheduler &scheduler;
        IJob &next;
        std::vector<JobId> dependsOn;
    };

}
