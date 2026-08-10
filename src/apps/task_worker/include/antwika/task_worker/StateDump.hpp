#pragma once

#include <nlohmann/json_fwd.hpp>

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include <antwika/replay/MigrationChain.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/task_worker/TaskRegistry.hpp"
#include "antwika/task_worker/Worker.hpp"

namespace antwika::task_worker
{

    inline constexpr std::string_view kStateDumpMagic =
        "antwika-task-worker-state-dump";

    inline constexpr std::uint32_t kStateDumpVersion = 1;

    struct WorkerDump final
    {
        WorkerStatus status{WorkerStatus::Idle};
        antwika::time::Tick remainingTicks{0};
        std::uint64_t taskId{0};
        std::string label;

        bool operator==(const WorkerDump &other) const = default;
    };

    struct SubmissionDump final
    {
        std::uint64_t taskId{0};
        std::string label;

        bool operator==(const SubmissionDump &other) const = default;
    };

    struct StateDump final
    {
        std::vector<WorkerDump> workers;
        std::vector<TaskInfo> tasks;
        std::vector<SubmissionDump> submissions;
        DispatchInfo dispatch;

        bool operator==(const StateDump &other) const = default;
    };

    [[nodiscard]] antwika::replay::MigrationChain
    standardStateDumpMigrations();

    [[nodiscard]] nlohmann::json stateDumpToJson(const StateDump &dump);

    [[nodiscard]] StateDump stateDumpFromJson(
        const nlohmann::json &state);

}
