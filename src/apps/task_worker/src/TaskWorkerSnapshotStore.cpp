#include "antwika/task_worker/TaskWorkerSnapshotStore.hpp"

#include <memory>
#include <utility>

#include <antwika/ecs_commons/Name.hpp>
#include <antwika/scheduler/JobId.hpp>

#include "antwika/task_worker/TaskJob.hpp"
#include "antwika/task_worker/Worker.hpp"

namespace antwika::task_worker
{

    TaskWorkerSnapshotStore::TaskWorkerSnapshotStore(
        World &world,
        std::vector<Entity> &workers,
        TaskRegistry &registry,
        TaskSubmissionSink &submissions,
        JobQueue &jobs,
        WorkerLookup &lookup) noexcept
        : antwika::console::IJsonSnapshotStore<
              antwika::console::SnapshotError>(
              {.magic = kStateDumpMagic,
               .version = kStateDumpVersion},
              "antwika task worker state dump document",
              standardStateDumpMigrations),
          world(world),
          workers(workers),
          registry(registry),
          submissions(submissions),
          jobs(jobs),
          lookup(lookup)
    {
    }

    nlohmann::json TaskWorkerSnapshotStore::takeState(
        const std::string &)
    {
        return stateDumpToJson(take());
    }

    void TaskWorkerSnapshotStore::applyState(
        const std::string &, const nlohmann::json &state)
    {
        apply(stateDumpFromJson(state));
    }

    StateDump TaskWorkerSnapshotStore::take() const
    {
        StateDump dump;

        for (const auto entity : workers)
        {
            const auto worker = world.get<Worker>(entity);
            dump.workers.push_back(WorkerDump{
                worker.status,
                worker.remainingTicks,
                worker.taskId,
                std::string(
                    antwika::ecs_commons::view(worker.label))});
        }

        dump.tasks = registry.allTasks();

        for (const auto &submission : submissions.submissions())
        {
            dump.submissions.push_back(SubmissionDump{
                submission.taskId, submission.label});
        }

        dump.dispatch = registry.lastDispatch();

        return dump;

    } // GCOVR_EXCL_LINE

    void TaskWorkerSnapshotStore::apply(const StateDump &dump)
    {
        for (const auto entity : workers)
        {
            world.destroy(entity);
        }

        workers.clear();
        for (const auto &worker : dump.workers)
        {
            const auto entity = world.create();
            world.add<Worker>(
                entity,
                Worker{
                    worker.status,
                    worker.remainingTicks,
                    worker.taskId,
                    antwika::ecs_commons::makeName(worker.label)});
            workers.push_back(entity);
        }

        world.commit();
        lookup.reset(workers);

        registry.restore(dump.tasks, dump.dispatch);

        auto &scheduler = jobs.reset();

        std::vector<std::pair<std::uint64_t, JobId>> renumbered;
        for (const auto &task : dump.tasks)
        {
            if (task.status != TaskStatus::Pending)
            {
                continue;
            }

            std::vector<JobId> dependsOn;
            if (task.dependsOn.has_value())
            {
                for (const auto &[taskId, jobId] : renumbered)
                {
                    if (taskId == task.dependsOn->taskId)
                    {
                        dependsOn.push_back(jobId);
                    }
                }
            }

            auto job = std::make_unique<TaskJob>(
                lookup, task.taskId, task.label, task.durationTicks);
            const auto jobId = scheduler.schedule(
                std::move(job), task.priority, std::move(dependsOn));
            renumbered.emplace_back(task.taskId, jobId);
        }

        std::vector<TaskSubmissionSink::Submission> accepted;
        for (const auto &submission : dump.submissions)
        {
            auto jobId = antwika::scheduler::kInvalidJobId;
            for (const auto &[taskId, renumberedId] : renumbered)
            {
                if (taskId == submission.taskId)
                {
                    jobId = renumberedId;
                }
            }

            accepted.push_back(TaskSubmissionSink::Submission{
                submission.taskId, jobId, submission.label});
        }

        submissions.restore(std::move(accepted));
    }

}
