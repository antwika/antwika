#pragma once

#include <cstdint>
#include <string>

#include <antwika/scheduler/IJob.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/task_worker/WorkerLookup.hpp"

namespace antwika::task_worker
{

    using antwika::scheduler::IJob;

    class TaskJob final : public IJob
    {
    public:
        TaskJob(
            WorkerLookup &lookup,
            std::uint64_t taskId,
            std::string label,
            antwika::time::Tick durationTicks);

        TaskJob(const TaskJob &) = delete;
        TaskJob(TaskJob &&) = delete;

        TaskJob &operator=(const TaskJob &) = delete;
        TaskJob &operator=(TaskJob &&) = delete;

        void execute(antwika::time::Tick tick) override;

    private:
        WorkerLookup &lookup;
        std::uint64_t id;
        std::string taskLabel;
        antwika::time::Tick durationTicks;
    };

}
