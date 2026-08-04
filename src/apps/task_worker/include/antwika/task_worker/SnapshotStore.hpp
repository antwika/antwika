#pragma once

#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include <antwika/console/JsonSnapshotStore.hpp>
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

    /**
     * @brief This application's half of dump_state and load_state.
     *
     * The full state is the Workers and the task bookkeeping; the
     * scheduler's pending queue is rebuilt rather than serialized,
     * because its jobs are callables by design -- see
     * wiki/libraries/scheduler.md.
     *
     * **A restore renumbers every JobId.** The old scheduler goes,
     * ids and pending jobs with it, and a fresh one re-issues 1 and
     * up to the Pending tasks in their original submission order.
     * The registry and the submission sink are both rebuilt with that
     * new numbering, which is what keeps markStarted() and a later
     * dependsOnId resolving to the right task afterwards.
     *
     * The state's own decoder already refuses with the seam's error,
     * so that is what this names as its own category and the seam's
     * rewrapping is the identity.
     */
    class TaskWorkerSnapshotStore final
        : public antwika::console::JsonSnapshotStore<
              antwika::console::SnapshotError>
    {
    public:
        /**
         * @brief Construct the store over everything a dump reads and
         * a load rebuilds.
         * @param world World the Worker entities live in.
         * @param workers The run's Worker entities, in creation
         * order; a load replaces the vector's contents in place.
         * @param registry Task registry a dump reads and a load
         * rebuilds.
         * @param submissions Submission sink whose accepted list a
         * dump reads and a load rebuilds.
         * @param jobs Holds the scheduler a load replaces.
         * @param lookup Worker lookup reset over the rebuilt pool.
         *
         * Every reference is borrowed and must outlive this store.
         */
        TaskWorkerSnapshotStore(
            World &world,
            std::vector<Entity> &workers,
            TaskRegistry &registry,
            TaskSubmissionSink &submissions,
            JobQueue &jobs,
            WorkerLookup &lookup) noexcept;

        /**
         * @brief Read the running state out, without writing it.
         * @return The state as a dump would hold it.
         */
        [[nodiscard]] StateDump take() const;

        /**
         * @brief Make the running state the dump's.
         * @param dump The state to rebuild from.
         */
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

} // namespace antwika::task_worker
