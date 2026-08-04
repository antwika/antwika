#include "antwika/task_worker/StateDump.hpp"

#include <array>
#include <cstddef>
#include <exception>
#include <optional>
#include <utility>

#include <nlohmann/json-schema.hpp>

#include <antwika/console/SnapshotError.hpp>
#include <antwika/scheduler/Priority.hpp>

namespace antwika::task_worker
{

    namespace
    {
        using antwika::console::SnapshotError;

        // The names a dump document holds, one per worker status.
        // Persisted, so they may not change once written.
        constexpr std::array<std::string_view, 2> kWorkerStatusNames{
            "idle", "busy"};

        // The names a dump document holds, one per task status.
        constexpr std::array<std::string_view, 3> kTaskStatusNames{
            "pending", "running", "completed"};

        [[nodiscard]] std::string_view workerStatusName(
            WorkerStatus status) noexcept
        {
            return kWorkerStatusNames
                [static_cast<std::size_t>(status)
                 % kWorkerStatusNames.size()];
        }

        [[nodiscard]] std::optional<WorkerStatus> workerStatusFromName(
            std::string_view name) noexcept
        {
            for (std::size_t index = 0;
                 index < kWorkerStatusNames.size();
                 ++index)
            {
                if (kWorkerStatusNames[index] == name)
                {
                    return static_cast<WorkerStatus>(index);
                }
            }

            return std::nullopt;
        }

        [[nodiscard]] std::string_view taskStatusName(
            TaskStatus status) noexcept
        {
            return kTaskStatusNames
                [static_cast<std::size_t>(status)
                 % kTaskStatusNames.size()];
        }

        [[nodiscard]] std::optional<TaskStatus> taskStatusFromName(
            std::string_view name) noexcept
        {
            for (std::size_t index = 0;
                 index < kTaskStatusNames.size();
                 ++index)
            {
                if (kTaskStatusNames[index] == name)
                {
                    return static_cast<TaskStatus>(index);
                }
            }

            return std::nullopt;
        }

        nlohmann::json stateSchema()
        {
            nlohmann::json schema;
            schema["$schema"] =
                "http://json-schema.org/draft-07/schema#";
            schema["title"] = "antwika task worker dump state";
            schema["type"] = "object";
            schema["additionalProperties"] = false;
            schema["required"] = {
                "workers",
                "tasks",
                "submissions",
                "dispatch"}; // GCOVR_EXCL_LINE

            auto &worker = schema["properties"]["workers"]["items"];
            schema["properties"]["workers"]["type"] = "array";
            worker["type"] = "object";
            worker["additionalProperties"] = false;
            worker["required"] = {
                "status",
                "remainingTicks",
                "taskId",
                "label"}; // GCOVR_EXCL_LINE
            worker["properties"]["status"]["type"] = "string";
            worker["properties"]["remainingTicks"]["type"] = "integer";
            worker["properties"]["remainingTicks"]["minimum"] = 0;
            worker["properties"]["taskId"]["type"] = "integer";
            worker["properties"]["taskId"]["minimum"] = 0;
            worker["properties"]["label"]["type"] = "string";

            auto &task = schema["properties"]["tasks"]["items"];
            schema["properties"]["tasks"]["type"] = "array";
            task["type"] = "object";
            task["additionalProperties"] = false;
            task["required"] = {
                "taskId",
                "label",
                "priority",
                "status",
                "durationTicks",
                "remainingTicks"}; // GCOVR_EXCL_LINE
            task["properties"]["taskId"]["type"] = "integer";
            task["properties"]["taskId"]["minimum"] = 0;
            task["properties"]["label"]["type"] = "string";
            task["properties"]["priority"]["type"] = "integer";
            task["properties"]["priority"]["minimum"] = 0;
            task["properties"]["priority"]["maximum"] = 255;
            task["properties"]["status"]["type"] = "string";
            task["properties"]["durationTicks"]["type"] = "integer";
            task["properties"]["durationTicks"]["minimum"] = 0;
            task["properties"]["remainingTicks"]["type"] = "integer";
            task["properties"]["remainingTicks"]["minimum"] = 0;
            task["properties"]["dependsOn"]["type"] = "integer";
            task["properties"]["dependsOn"]["minimum"] = 0;

            auto &entry = schema["properties"]["submissions"]["items"];
            schema["properties"]["submissions"]["type"] = "array";
            entry["type"] = "object";
            entry["additionalProperties"] = false;
            entry["required"] = {"taskId", "label"}; // GCOVR_EXCL_LINE
            entry["properties"]["taskId"]["type"] = "integer";
            entry["properties"]["taskId"]["minimum"] = 0;
            entry["properties"]["label"]["type"] = "string";

            auto &dispatch = schema["properties"]["dispatch"];
            dispatch["type"] = "object";
            dispatch["additionalProperties"] = false;
            dispatch["required"] = {
                "budget", "dispatched"}; // GCOVR_EXCL_LINE
            dispatch["properties"]["budget"]["type"] = "integer";
            dispatch["properties"]["budget"]["minimum"] = 0;
            dispatch["properties"]["dispatched"]["type"] = "integer";
            dispatch["properties"]["dispatched"]["minimum"] = 0;

            return schema;
        }

        const nlohmann::json_schema::json_validator &stateValidator()
        {
            // The excluded closing line carries the static guard.
            // Its concurrency arms are unreachable one-threaded.
            // See docs/confirming-unreachable-branches.md.
            static const nlohmann::json_schema::json_validator validator(
                stateSchema()); // GCOVR_EXCL_LINE
            return validator;
        }

        [[nodiscard]] const TaskInfo *taskWithId(
            const std::vector<TaskInfo> &tasks,
            std::uint64_t taskId) noexcept
        {
            for (const auto &task : tasks)
            {
                if (task.taskId == taskId)
                {
                    return &task;
                }
            }

            return nullptr;
        }
    } // namespace

    antwika::replay::MigrationChain standardStateDumpMigrations()
    {
        // Empty until a version 2 exists.
        // That is how every format here starts.
        return antwika::replay::MigrationChain({}, kStateDumpVersion);
    }

    nlohmann::json stateDumpToJson(const StateDump &dump)
    {
        nlohmann::json encoded;

        encoded["workers"] = nlohmann::json::array();
        for (const auto &worker : dump.workers)
        {
            nlohmann::json entry;
            entry["status"] =
                std::string(workerStatusName(worker.status));
            entry["remainingTicks"] = worker.remainingTicks;
            entry["taskId"] = worker.taskId;
            entry["label"] = worker.label;
            encoded["workers"].push_back(std::move(entry));
        }

        encoded["tasks"] = nlohmann::json::array();
        for (const auto &task : dump.tasks)
        {
            nlohmann::json entry;
            entry["taskId"] = task.taskId;
            entry["label"] = task.label;
            entry["priority"] =
                antwika::scheduler::rawValue(task.priority);
            entry["status"] = std::string(taskStatusName(task.status));
            entry["durationTicks"] = task.durationTicks;
            entry["remainingTicks"] = task.remainingTicks;

            // Absent means the task waits on nothing.
            // A member for it would be an id for no task.
            if (task.dependsOn.has_value())
            {
                entry["dependsOn"] = task.dependsOn->taskId;
            }

            encoded["tasks"].push_back(std::move(entry));
        }

        encoded["submissions"] = nlohmann::json::array();
        for (const auto &submission : dump.submissions)
        {
            nlohmann::json entry;
            entry["taskId"] = submission.taskId;
            entry["label"] = submission.label;
            encoded["submissions"].push_back(std::move(entry));
        }

        encoded["dispatch"]["budget"] = dump.dispatch.budget;
        encoded["dispatch"]["dispatched"] = dump.dispatch.dispatched;

        return encoded;

        // gcov puts the returned value's unwind block here.
        // Nothing between its construction and the return throws.
    } // GCOVR_EXCL_LINE

    StateDump stateDumpFromJson(const nlohmann::json &state)
    {
        try
        {
            stateValidator().validate(state);
        }
        // The validator's failure type is the library's business.
        // What this format promises is SnapshotError.
        catch (const std::exception &failed) // GCOVR_EXCL_LINE
        {
            throw SnapshotError(
                std::string(
                    "antwika::task_worker: dump state failed schema "
                    "validation: ")
                + failed.what());
        }

        StateDump dump;

        for (const auto &entry : state.at("workers"))
        {
            const auto named =
                entry.at("status").get<std::string>();
            const auto status = workerStatusFromName(named);

            if (!status.has_value())
            {
                throw SnapshotError(
                    "antwika::task_worker: dump names a worker "
                    "status this build does not know: "
                    + named);
            }

            dump.workers.push_back(WorkerDump{
                *status,
                entry.at("remainingTicks")
                    .get<antwika::time::Tick>(),
                entry.at("taskId").get<std::uint64_t>(),
                entry.at("label").get<std::string>()});
        }

        for (const auto &entry : state.at("tasks"))
        {
            const auto named =
                entry.at("status").get<std::string>();
            const auto status = taskStatusFromName(named);

            if (!status.has_value())
            {
                throw SnapshotError(
                    "antwika::task_worker: dump names a task status "
                    "this build does not know: "
                    + named);
            }

            dump.tasks.push_back(TaskInfo{
                entry.at("taskId").get<std::uint64_t>(),
                entry.at("label").get<std::string>(),
                static_cast<antwika::scheduler::Priority>(
                    entry.at("priority").get<std::uint64_t>()),
                *status,
                entry.at("durationTicks").get<antwika::time::Tick>(),
                entry.at("remainingTicks")
                    .get<antwika::time::Tick>(),
                std::nullopt});
        }

        // Resolved in a second pass.
        // A label is read off the list it must name a task in.
        std::size_t index = 0;
        for (const auto &entry : state.at("tasks"))
        {
            if (entry.contains("dependsOn"))
            {
                const auto dependsOnTaskId =
                    entry.at("dependsOn").get<std::uint64_t>();
                const auto *found =
                    taskWithId(dump.tasks, dependsOnTaskId);

                if (found == nullptr)
                {
                    throw SnapshotError(
                        "antwika::task_worker: dump has a task "
                        "depend on a taskId no task carries");
                }

                dump.tasks[index].dependsOn =
                    TaskDependency{dependsOnTaskId, found->label};
            }

            ++index;
        }

        for (const auto &entry : state.at("submissions"))
        {
            dump.submissions.push_back(SubmissionDump{
                entry.at("taskId").get<std::uint64_t>(),
                entry.at("label").get<std::string>()});
        }

        dump.dispatch = DispatchInfo{
            state.at("dispatch").at("budget").get<std::size_t>(),
            state.at("dispatch").at("dispatched").get<std::size_t>()};

        return dump;
    }

} // namespace antwika::task_worker
