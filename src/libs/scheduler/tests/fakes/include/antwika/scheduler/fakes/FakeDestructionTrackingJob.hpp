#pragma once

#include <antwika/time/Tick.hpp>

#include "antwika/scheduler/IJob.hpp"

namespace antwika::scheduler::fakes
{

    class FakeDestructionTrackingJob final : public IJob
    {
    public:
        explicit FakeDestructionTrackingJob(bool &destroyed)
            : destroyed(destroyed)
        {
        }

        ~FakeDestructionTrackingJob() override
        {
            destroyed = true;
        }

        FakeDestructionTrackingJob(
            const FakeDestructionTrackingJob &) = delete;
        FakeDestructionTrackingJob(FakeDestructionTrackingJob &&) = delete;

        FakeDestructionTrackingJob &operator=(
            const FakeDestructionTrackingJob &) = delete;
        FakeDestructionTrackingJob &operator=(
            FakeDestructionTrackingJob &&) = delete;

        void execute(antwika::time::Tick) override
        {
        }

    private:
        bool &destroyed;
    };

}
