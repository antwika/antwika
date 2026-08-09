#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include <antwika/time/Tick.hpp>

#include "antwika/scheduler/IJob.hpp"
#include "antwika/scheduler/JobId.hpp"
#include "antwika/scheduler/Priority.hpp"

namespace antwika::scheduler
{

    class Scheduler final
    {
    public:
        Scheduler() = default;

        Scheduler(const Scheduler &) = delete;
        Scheduler(Scheduler &&) = delete;

        Scheduler &operator=(const Scheduler &) = delete;
        Scheduler &operator=(Scheduler &&) = delete;

        JobId schedule(
            std::unique_ptr<IJob> job,
            Priority priority,
            std::vector<JobId> dependsOn = {});

        JobId schedule(
            IJob &job,
            Priority priority,
            std::vector<JobId> dependsOn = {});

        std::vector<JobId> run(antwika::time::Tick tick, std::size_t budget);

        [[nodiscard]] std::size_t pending() const noexcept;

        [[nodiscard]] bool empty() const noexcept;

    private:
        struct Entry final
        {
            JobId id;
            Priority priority;
        };

        struct JobRecord final
        {
            Priority priority;
            IJob *job;
            bool completed;
        };

        void normaliseDependencies(std::vector<JobId> &dependsOn) const;

        JobId scheduleValidated(
            IJob &job,
            Priority priority,
            std::vector<JobId> dependsOn);

        void insertReady(JobId id, Priority priority);

        void finish(std::size_t index);

        std::vector<std::unique_ptr<IJob>> ownedJobs;
        std::vector<Entry> ready;
        std::vector<JobRecord> records;
        std::vector<std::size_t> unmetCount;
        std::vector<std::vector<JobId>> dependents;

        std::size_t pendingCount{0};
        JobId nextId{1};
    };

}
