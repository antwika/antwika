#pragma once

#include <string>
#include <vector>

#include <antwika/console/ISnapshotStore.hpp>
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
     */
    class TaskWorkerSnapshotStore final
        : public antwika::console::ISnapshotStore
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
         * @brief Write the running state to a file.
         * @param path Where to write it.
         * @param console The console's history, carried in the dump.
         * @throws antwika::console::SnapshotError If the file cannot
         * be written.
         */
        void dump(
            const std::string &path,
            const std::vector<std::string> &console) override;

        /**
         * @brief Read a file and rebuild the pool from it.
         * @param path The file to read.
         * @return The console history the dump carried.
         * @throws antwika::console::SnapshotError If the file is not
         * there, is not this application's dump, or cannot be applied.
         */
        [[nodiscard]] std::vector<std::string> load(
            const std::string &path) override;

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
        World &world;
        std::vector<Entity> &workers;
        TaskRegistry &registry;
        TaskSubmissionSink &submissions;
        JobQueue &jobs;
        WorkerLookup &lookup;
    };

} // namespace antwika::task_worker
