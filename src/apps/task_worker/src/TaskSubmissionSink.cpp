#include "antwika/task_worker/TaskSubmissionSink.hpp"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <antwika/engine/Events.hpp>
#include <antwika/replay/JsonShapes.hpp>
#include <antwika/replay/PayloadJson.hpp>
#include <antwika/scheduler/Priority.hpp>

#include "antwika/task_worker/Events.hpp"
#include "antwika/task_worker/TaskSubmissionError.hpp"

namespace antwika::task_worker
{

    namespace
    {

        nlohmann::json taskSubmitSchema()
        {
            nlohmann::json schema = antwika::replay::documentShape(
                "task.submit payload",
                {"id", "priority", "durationTicks", "label"});
            schema["properties"]["id"] = antwika::replay::countShape();
            schema["properties"]["priority"] =
                antwika::replay::boundedCountShape(255);

            // At least one tick, since a task that takes none is done.
            schema["properties"]["durationTicks"]["type"] = "integer";
            schema["properties"]["durationTicks"]["minimum"] = 1;
            schema["properties"]["label"] = antwika::replay::wordShape();
            schema["properties"]["dependsOnId"] =
                antwika::replay::countShape();
            return schema;
        }

    } // namespace

    TaskSubmissionSink::TaskSubmissionSink(
        World &world,
        SystemScheduler &systemScheduler,
        JobQueue &jobs,
        WorkerLookup &lookup,
        TaskRegistry &registry)
        : world(world),
          systemScheduler(systemScheduler),
          jobs(jobs),
          lookup(lookup),
          registry(registry)
    {
    }

    const std::vector<TaskSubmissionSink::Submission> &
    TaskSubmissionSink::submissions() const noexcept
    {
        return submitted;
    }

    void TaskSubmissionSink::restore(
        std::vector<Submission> replacement)
    {
        submitted = std::move(replacement);
    }

    void TaskSubmissionSink::handle(const TickEvent &event)
    {
        if (event.event.name == antwika::engine::events::kTick)
        {
            world.commit();
            systemScheduler.run(world, event.tick);
            return;
        }

        if (event.event.name != events::kTaskSubmit)
        {
            return;
        }

        const auto parsed =
            antwika::replay::parseAndValidatePayload<TaskSubmissionError>(
                event.event.payload,
                antwika::replay::validatorFor<taskSubmitSchema>(),
                "TaskSubmissionSink: task.submit payload");

        const auto taskId = parsed.at("id").get<std::uint64_t>();
        const auto alreadySubmitted = std::find_if(
            submitted.begin(),
            submitted.end(),
            [taskId](const auto &entry) { return entry.taskId == taskId; });
        if (alreadySubmitted != submitted.end())
        {
            throw TaskSubmissionError(
                "TaskSubmissionSink: task.submit payload's id was "
                "already submitted");
        }
        const auto priority = static_cast<antwika::scheduler::Priority>(
            parsed.at("priority").get<std::uint64_t>());
        const auto durationTicks =
            parsed.at("durationTicks").get<std::uint64_t>();
        auto label = parsed.at("label").get<std::string>();

        std::vector<JobId> dependsOn;
        std::optional<TaskDependency> dependencyInfo;
        const auto dependsOnIt = parsed.find("dependsOnId");
        if (dependsOnIt != parsed.end())
        {
            const auto dependsOnTaskId =
                dependsOnIt->get<std::uint64_t>();
            const auto found = std::find_if(
                submitted.begin(),
                submitted.end(),
                [dependsOnTaskId](const auto &entry)
                { return entry.taskId == dependsOnTaskId; });
            if (found == submitted.end())
            {
                throw TaskSubmissionError(
                    "TaskSubmissionSink: dependsOnId refers to a task "
                    "id that was never submitted");
            }
            // A restore marks a started or finished task invalid.
            // Its dependency edge is satisfied by definition.
            // So no edge is scheduled for it.
            if (found->jobId != antwika::scheduler::kInvalidJobId)
            {
                dependsOn.push_back(found->jobId);
            }
            dependencyInfo =
                TaskDependency{dependsOnTaskId, found->label};
        }

        auto job = std::make_unique<TaskJob>(
            lookup, taskId, label, durationTicks);
        const auto jobId = jobs.scheduler().schedule(
            std::move(job), priority, dependsOn);
        registry.submit(
            taskId, label, priority, durationTicks, dependencyInfo);
        submitted.push_back(
            Submission{taskId, jobId, std::move(label)});
    }

} // namespace antwika::task_worker
