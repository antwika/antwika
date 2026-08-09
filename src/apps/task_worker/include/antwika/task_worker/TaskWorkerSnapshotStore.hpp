#pragma once

#include <nlohmann/json.hpp>

#include <string>
#include <vector>

#include <antwika/console/IJsonSnapshotStore.hpp>
#include <antwika/console/SnapshotError.hpp>
#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/World.hpp>

#include "antwika/task_worker/JobQueue.hpp"
#include "antwika/task_worker/StateDump.hpp"
#include "antwika/task_worker/TaskRegistry.hpp"
#include "antwika/task_worker/TaskSubmissionSink.hpp"
#include "antwika/task_worker/WorkerLookup.hpp"

namespace antwika::task_worker
{

    using antwika::ecs::Entity;
    using antwika::ecs::World;

    class TaskWorkerSnapshotStore final
        : public antwika::console::IJsonSnapshotStore<
              antwika::console::SnapshotError>
    {
    public:
        TaskWorkerSnapshotStore(
            World &world,
            std::vector<Entity> &workers,
            TaskRegistry &registry,
            TaskSubmissionSink &submissions,
            JobQueue &jobs,
            WorkerLookup &lookup) noexcept;

        [[nodiscard]] StateDump take() const;

        void apply(const StateDump &dump);

    private:
        [[nodiscard]] nlohmann::json takeState(
            const std::string &path) override;

        void applyState(
            const std::string &path,
            const nlohmann::json &state) override;

        World &world;
        std::vector<Entity> &workers;
        TaskRegistry &registry;
        TaskSubmissionSink &submissions;
        JobQueue &jobs;
        WorkerLookup &lookup;
    };

}
