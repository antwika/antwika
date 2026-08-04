#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include <antwika/replay/MigrationChain.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/task_worker/TaskRegistry.hpp"
#include "antwika/task_worker/Worker.hpp"

namespace antwika::task_worker
{

    /** @brief The magic naming this application's dump files. */
    inline constexpr std::string_view kStateDumpMagic =
        "antwika-task-worker-state-dump";

    /** @brief The dump document version this build writes and reads. */
    inline constexpr std::uint32_t kStateDumpVersion = 1;

    /**
     * @brief One Worker as a dump document holds it.
     *
     * A plain struct rather than the component, because the component
     * carries its label as a fixed ecs_commons::Name and a document
     * holds a string.
     */
    struct WorkerDump
    {
        WorkerStatus status{WorkerStatus::Idle};
        antwika::time::Tick remainingTicks{0};
        std::uint64_t taskId{0};
        std::string label;

        bool operator==(const WorkerDump &other) const = default;
    };

    /**
     * @brief One accepted task.submit as a dump document holds it.
     *
     * Deliberately without its JobId: those are the scheduler's, are
     * renumbered by every restore, and would be stale the moment they
     * were written -- see TaskWorkerSnapshotStore.
     */
    struct SubmissionDump
    {
        std::uint64_t taskId{0};
        std::string label;

        bool operator==(const SubmissionDump &other) const = default;
    };

    /**
     * @brief The whole running state, as dump_state writes it.
     *
     * The Workers and the task bookkeeping are the state; the
     * scheduler's pending queue is deliberately absent, because its
     * jobs are callables by design and a restore rebuilds it from the
     * Pending tasks instead.
     */
    struct StateDump
    {
        std::vector<WorkerDump> workers;
        std::vector<TaskInfo> tasks;
        std::vector<SubmissionDump> submissions;
        DispatchInfo dispatch;

        bool operator==(const StateDump &other) const = default;
    };

    /**
     * @brief Build the migration chain a dump document is read up
     * through.
     * @return The chain; empty, at version 1, until a version 2 exists.
     */
    [[nodiscard]] antwika::replay::MigrationChain
    standardStateDumpMigrations();

    /**
     * @brief Encode the running state as a dump document's state
     * member.
     * @param dump The state to encode.
     * @return The encoded object.
     */
    [[nodiscard]] nlohmann::json stateDumpToJson(const StateDump &dump);

    /**
     * @brief Decode a dump document's state member, validating it
     * first.
     * @param state The state object to read.
     * @return The decoded state.
     * @throws antwika::console::SnapshotError If the object fails the
     * schema, names a status this build does not know, or has a task
     * depend on a taskId no task in the document carries.
     */
    [[nodiscard]] StateDump stateDumpFromJson(
        const nlohmann::json &state);

} // namespace antwika::task_worker
