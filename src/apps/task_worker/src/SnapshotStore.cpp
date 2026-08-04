#include "antwika/task_worker/SnapshotStore.hpp"

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
        : antwika::console::JsonSnapshotStore<
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

        // gcov puts the returned value's unwind block here.
        // No input reaches it.
    } // GCOVR_EXCL_LINE

    void TaskWorkerSnapshotStore::apply(const StateDump &dump)
    {
        // The pool becomes the dump's, entity for entity.
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

        // One commit lands the destroys and the adds together.
        world.commit();
        lookup.reset(workers);

        // The registry first.
        // restore() records the numbering handed out just below.
        // Pending tasks in original order, against JobIds 1 and up.
        registry.restore(dump.tasks, dump.dispatch);

        // The old queue goes, its pending callables with it.
        auto &scheduler = jobs.reset();

        // A vector of pairs rather than a map.
        // The walk below is then ordered by construction.
        std::vector<std::pair<std::uint64_t, JobId>> renumbered;
        for (const auto &task : dump.tasks)
        {
            if (task.status != TaskStatus::Pending)
            {
                continue;
            }

            // Only an edge to a still-Pending task is re-expressed.
            // One to a started or finished task is satisfied already.
            // A dependency names an earlier submission.
            // So its new id is listed before its dependent schedules.
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

        // The sink's list carries the new numbering too.
        // A later task.submit resolves its dependsOnId against it.
        // A task no longer pending is marked kInvalidJobId.
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

} // namespace antwika::task_worker
