#include "antwika/task_worker/TaskSubmissionSink.hpp"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <nlohmann/json-schema.hpp>

#include <antwika/engine/Events.hpp>
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
            nlohmann::json schema;
            schema["$schema"] = "http://json-schema.org/draft-07/schema#";
            schema["title"] = "task.submit payload";
            schema["type"] = "object";
            schema["additionalProperties"] = false;
            schema["required"] =
                {"id", "priority", "durationTicks", "label"}; // GCOVR_EXCL_LINE
            schema["properties"]["id"]["type"] = "integer";
            schema["properties"]["id"]["minimum"] = 0;
            schema["properties"]["priority"]["type"] = "integer";
            schema["properties"]["priority"]["minimum"] = 0;
            schema["properties"]["priority"]["maximum"] = 255;
            schema["properties"]["durationTicks"]["type"] = "integer";
            schema["properties"]["durationTicks"]["minimum"] = 1;
            schema["properties"]["label"]["type"] = "string";
            schema["properties"]["dependsOnId"]["type"] = "integer";
            schema["properties"]["dependsOnId"]["minimum"] = 0;
            return schema;
        }

        const nlohmann::json_schema::json_validator &taskSubmitValidator()
        {
            static const nlohmann::json_schema::json_validator validator(
                taskSubmitSchema()); // GCOVR_EXCL_LINE
            return validator;
        }

    } // namespace

    TaskSubmissionSink::TaskSubmissionSink(
        World &world,
        SystemScheduler &systemScheduler,
        Scheduler &jobScheduler,
        WorkerLookup &lookup,
        TaskRegistry &registry)
        : world(world),
          systemScheduler(systemScheduler),
          jobScheduler(jobScheduler),
          lookup(lookup),
          registry(registry)
    {
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
                taskSubmitValidator(),
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
            dependsOn.push_back(found->jobId);
            dependencyInfo =
                TaskDependency{dependsOnTaskId, found->label};
        }

        auto job = std::make_unique<TaskJob>(
            lookup, taskId, label, durationTicks);
        const auto jobId = jobScheduler.schedule(
            std::move(job), priority, dependsOn);
        registry.submit(
            taskId, label, priority, durationTicks, dependencyInfo);
        submitted.push_back(
            Submission{taskId, jobId, std::move(label)});
    }

} // namespace antwika::task_worker
