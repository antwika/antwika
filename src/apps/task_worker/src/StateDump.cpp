#include "antwika/task_worker/StateDump.hpp"

#include <cstddef>
#include <exception>
#include <utility>

#include <antwika/console/SnapshotError.hpp>
#include <antwika/replay/JsonShapes.hpp>
#include <antwika/enums/FromName.hpp>
#include <antwika/scheduler/Priority.hpp>

namespace antwika::task_worker
{

    namespace
    {
        using antwika::console::SnapshotError;
        using antwika::replay::countShape;
        using antwika::replay::objectShape;
        using antwika::replay::wordShape;

        constexpr antwika::enums::NameTable<WorkerStatus>
            kWorkerStatuses{{"idle", "busy"}};

        constexpr antwika::enums::NameTable<TaskStatus>
            kTaskStatuses{{"pending", "running", "completed"}};

        nlohmann::json stateSchema()
        {
            nlohmann::json schema = antwika::replay::documentShape(
                "antwika task worker dump state",
                {"workers", "tasks", "submissions", "dispatch"});

            auto &workers = schema["properties"]["workers"];
            workers["type"] = "array";
            workers["items"] = objectShape(
                {"status", "remainingTicks", "taskId", "label"});

            auto &worker = workers["items"];
            worker["properties"]["status"] = wordShape();
            worker["properties"]["remainingTicks"] = countShape();
            worker["properties"]["taskId"] = countShape();
            worker["properties"]["label"] = wordShape();

            auto &tasks = schema["properties"]["tasks"];
            tasks["type"] = "array";
            tasks["items"] = objectShape(
                {"taskId",
                 "label",
                 "priority",
                 "status",
                 "durationTicks",
                 "remainingTicks"});

            auto &task = tasks["items"];
            task["properties"]["taskId"] = countShape();
            task["properties"]["label"] = wordShape();
            task["properties"]["priority"] =
                antwika::replay::boundedCountShape(255);
            task["properties"]["status"] = wordShape();
            task["properties"]["durationTicks"] = countShape();
            task["properties"]["remainingTicks"] = countShape();
            task["properties"]["dependsOn"] = countShape();

            auto &submissions = schema["properties"]["submissions"];
            submissions["type"] = "array";
            submissions["items"] = objectShape({"taskId", "label"});

            auto &entry = submissions["items"];
            entry["properties"]["taskId"] = countShape();
            entry["properties"]["label"] = wordShape();

            auto &dispatch = schema["properties"]["dispatch"];
            dispatch = objectShape({"budget", "dispatched"});
            dispatch["properties"]["budget"] = countShape();
            dispatch["properties"]["dispatched"] = countShape();

            return schema;
        } // GCOVR_EXCL_LINE

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
    }

    antwika::replay::MigrationChain standardStateDumpMigrations()
    {
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
                std::string(kWorkerStatuses.name(worker.status));
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
            entry["status"] =
                std::string(kTaskStatuses.name(task.status));
            entry["durationTicks"] = task.durationTicks;
            entry["remainingTicks"] = task.remainingTicks;

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

    } // GCOVR_EXCL_LINE

    StateDump stateDumpFromJson(const nlohmann::json &state)
    {
        try
        {
            antwika::replay::validatorFor<stateSchema>().validate(state);
        }
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
            const auto status =
                antwika::enums::fromName<SnapshotError>(
                    kWorkerStatuses,
                    entry.at("status").get<std::string>(),
                    "antwika::task_worker: dump names a worker status "
                    "this build does not know: ");

            dump.workers.push_back(WorkerDump{
                status,
                entry.at("remainingTicks")
                    .get<antwika::time::Tick>(),
                entry.at("taskId").get<std::uint64_t>(),
                entry.at("label").get<std::string>()});
        }

        for (const auto &entry : state.at("tasks"))
        {
            const auto status =
                antwika::enums::fromName<SnapshotError>(
                    kTaskStatuses,
                    entry.at("status").get<std::string>(),
                    "antwika::task_worker: dump names a task status "
                    "this build does not know: ");

            dump.tasks.push_back(TaskInfo{ // GCOVR_EXCL_LINE
                entry.at("taskId").get<std::uint64_t>(),
                entry.at("label").get<std::string>(),
                static_cast<antwika::scheduler::Priority>(
                    entry.at("priority").get<std::uint64_t>()),
                status,
                entry.at("durationTicks").get<antwika::time::Tick>(),
                entry.at("remainingTicks")
                    .get<antwika::time::Tick>(),
                std::nullopt});
        }

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

}
