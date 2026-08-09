#pragma once

#include <memory>

#include <antwika/scheduler/Scheduler.hpp>

namespace antwika::task_worker
{

    using antwika::scheduler::Scheduler;

    class JobQueue final
    {
    public:
        JobQueue();

        JobQueue(const JobQueue &) = delete;
        JobQueue(JobQueue &&) = delete;

        JobQueue &operator=(const JobQueue &) = delete;
        JobQueue &operator=(JobQueue &&) = delete;

        [[nodiscard]] Scheduler &scheduler() noexcept;

        Scheduler &reset();

    private:
        std::unique_ptr<Scheduler> current;
    };

}
