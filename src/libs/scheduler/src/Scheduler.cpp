#include "antwika/scheduler/Scheduler.hpp"

#include <algorithm>

#include "antwika/scheduler/SchedulerError.hpp"

namespace antwika::scheduler
{

    namespace
    {

        [[nodiscard]] bool runsBefore(
            Priority leftPriority,
            JobId leftId,
            Priority rightPriority,
            JobId rightId) noexcept
        {
            if (rawValue(leftPriority) != rawValue(rightPriority))
            {
                return rawValue(leftPriority) > rawValue(rightPriority);
            }

            return rawValue(leftId) < rawValue(rightId);
        }

    } // namespace

    JobId Scheduler::schedule(
        IJob &job,
        Priority priority,
        std::vector<JobId> dependsOn)
    {
        std::sort(
            dependsOn.begin(),
            dependsOn.end(),
            [](JobId left, JobId right)
            { return rawValue(left) < rawValue(right); });
        dependsOn.erase(
            std::unique(dependsOn.begin(), dependsOn.end()),
            dependsOn.end());

        const auto newId = nextId;
        const auto newRaw = rawValue(newId);

        for (const auto dependency : dependsOn)
        {
            const auto dependencyRaw = rawValue(dependency);
            if (dependencyRaw == 0 || dependencyRaw >= newRaw)
            {
                throw SchedulerError(
                    "Scheduler::schedule: dependsOn contains an "
                    "unknown JobId");
            }
        }

        std::size_t unmet = 0;
        for (const auto dependency : dependsOn)
        {
            const auto dependencyIndex = rawValue(dependency) - 1;
            if (!records[dependencyIndex].completed)
            {
                ++unmet;
            }
        }

        nextId = static_cast<JobId>(newRaw + 1);
        records.push_back(JobRecord{priority, &job, false});
        unmetCount.push_back(unmet);
        dependents.emplace_back();
        ++pendingCount;

        if (unmet == 0)
        {
            insertReady(newId, priority);
        }
        else
        {
            for (const auto dependency : dependsOn)
            {
                const auto dependencyIndex = rawValue(dependency) - 1;
                if (!records[dependencyIndex].completed)
                {
                    dependents[dependencyIndex].push_back(newId);
                }
            }
        }

        return newId;
    }

    std::vector<JobId> Scheduler::run(
        antwika::time::Tick tick,
        std::size_t budget)
    {
        const auto epoch = rawValue(nextId);
        std::vector<JobId> executed;

        while (executed.size() < budget)
        {
            const auto it = std::find_if(
                ready.begin(),
                ready.end(),
                [epoch](const Entry &entry)
                { return rawValue(entry.id) < epoch; });

            if (it == ready.end())
            {
                break;
            }

            const auto id = it->id;
            ready.erase(it);

            const auto index = rawValue(id) - 1;
            records[index].job->execute(tick);
            records[index].completed = true;
            --pendingCount;
            executed.push_back(id);

            for (const auto dependentId : dependents[index])
            {
                const auto dependentIndex = rawValue(dependentId) - 1;
                --unmetCount[dependentIndex];
                if (unmetCount[dependentIndex] == 0)
                {
                    insertReady(
                        dependentId,
                        records[dependentIndex].priority);
                }
            }
            dependents[index].clear();
        }

        return executed;
    } // GCOVR_EXCL_LINE

    std::size_t Scheduler::pending() const noexcept
    {
        return pendingCount;
    }

    bool Scheduler::empty() const noexcept
    {
        return pendingCount == 0;
    }

    void Scheduler::insertReady(JobId id, Priority priority)
    {
        const auto pos = std::upper_bound(
            ready.begin(),
            ready.end(),
            Entry{id, priority},
            [](const Entry &left, const Entry &right)
            {
                return runsBefore(
                    left.priority, left.id, right.priority, right.id);
            });
        ready.insert(pos, Entry{id, priority});
    }

} // namespace antwika::scheduler
