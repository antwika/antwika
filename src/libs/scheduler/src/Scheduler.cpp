#include "antwika/scheduler/Scheduler.hpp"

#include <algorithm>
#include <limits>
#include <memory>
#include <utility>

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
        std::unique_ptr<IJob> job,
        Priority priority,
        std::vector<JobId> dependsOn)
    {
        if (job == nullptr)
        {
            throw SchedulerError(
                "Scheduler::schedule: job must not be null");
        }

        // Validated before ownership moves, so there is nothing to undo.
        // A rejected job dies with this parameter on the way out.
        // Undoing it afterwards would need a catch clause instead.
        // Such a clause would also see allocation failures.
        // Those can strike once the job is already half-recorded.
        // Untangling that is not what a rejected dependency needs.
        normaliseDependencies(dependsOn);

        IJob &borrowed = *job;
        const auto id = scheduleValidated(
            borrowed, priority, std::move(dependsOn));

        // ownedJobs runs in lockstep with records.
        // There is one slot per JobId, empty for a borrowed job.
        // That is what lets finishing a job free it by its own id.
        ownedJobs[rawValue(id) - 1] = std::move(job);
        return id;
    }

    void Scheduler::normaliseDependencies(
        std::vector<JobId> &dependsOn) const
    {
        std::sort(
            dependsOn.begin(),
            dependsOn.end(),
            [](JobId left, JobId right)
            { return rawValue(left) < rawValue(right); });
        dependsOn.erase(
            std::unique(dependsOn.begin(), dependsOn.end()),
            dependsOn.end());

        const auto newRaw = rawValue(nextId);

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
    }

    JobId Scheduler::schedule(
        IJob &job,
        Priority priority,
        std::vector<JobId> dependsOn)
    {
        normaliseDependencies(dependsOn);
        return scheduleValidated(job, priority, std::move(dependsOn));
    }

    JobId Scheduler::scheduleValidated(
        IJob &job,
        Priority priority,
        std::vector<JobId> dependsOn)
    {
        const auto newId = nextId;
        const auto newRaw = rawValue(newId);

        // The row vectors first, the graph wiring last.
        // Growing a vector can throw.
        // A throw between the two must not leave a ghost JobId.
        // Wired into dependents with no record behind it, that is.
        // The count starts at never-ready rather than zero.
        // A throw mid-wiring then parks the job forever.
        // Better than running with half its dependencies unrecorded.
        records.push_back(JobRecord{priority, &job, false});
        ownedJobs.emplace_back();
        unmetCount.push_back(std::numeric_limits<std::size_t>::max());
        dependents.emplace_back();

        // The id is spent once its row exists.
        // Reusing it after a throw would misalign every row after.
        nextId = static_cast<JobId>(newRaw + 1);
        ++pendingCount;

        std::size_t unmet = 0;
        for (const auto dependency : dependsOn)
        {
            const auto dependencyIndex = rawValue(dependency) - 1;
            if (!records[dependencyIndex].completed)
            {
                ++unmet;
                dependents[dependencyIndex].push_back(newId);
            }
        }

        unmetCount.back() = unmet;

        if (unmet == 0)
        {
            insertReady(newId, priority);
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
            // The whole ready list, from its front, on every pop.
            // Deliberate, not an oversight.
            // A completion may ready a job behind a carried cursor.
            // And the list is priority-ordered from the front.
            // O(budget x ready), small for every in-tree budget.
            // A cursor would trade correctness for that cost.
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
            try
            {
                records[index].job->execute(tick);
            }
            catch (...)
            {
                // A job that throws has still had its turn.
                // Left half-run it would be gone from ready.
                // pending() would still count it.
                // Its dependents would wait on it forever.
                // So it finishes here exactly as a clean job does.
                // The exception then carries on to the caller.
                finish(index);
                throw;
            }

            finish(index);
            executed.push_back(id);
        }

        return executed;
    } // GCOVR_EXCL_LINE

    void Scheduler::finish(std::size_t index)
    {
        records[index].completed = true;
        --pendingCount;

        for (const auto dependentId : dependents[index])
        {
            const auto dependentIndex = rawValue(dependentId) - 1;
            --unmetCount[dependentIndex];
            if (unmetCount[dependentIndex] == 0)
            {
                insertReady(
                    dependentId, records[dependentIndex].priority);
            }
        }

        // A completed job is never looked up again.
        // An empty vector hands the memory back; clear() would not.
        dependents[index] = {};

        // The row itself has to stay, since a JobId indexes it.
        // What need not stay is the job, which can be the bulk of it.
        // Its pointer goes with it so nothing can follow it later.
        records[index].job = nullptr;
        ownedJobs[index].reset();
    }

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
