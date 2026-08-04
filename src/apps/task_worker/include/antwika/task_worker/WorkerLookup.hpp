#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/time/Tick.hpp>

namespace antwika::task_worker
{

    using antwika::ecs::Entity;
    using antwika::ecs::World;

    /**
     * @brief Tracks which of a fixed set of Worker entities is idle,
     * kept consistent across every TaskJob claim within one tick's
     * dispatch, not just across ticks.
     *
     * World::set() only becomes visible via World::get()/view() after
     * the owning phase commits, so multiple TaskJobs claiming workers
     * within the same Scheduler::run() call (mid-phase, pre-commit)
     * can't tell each other's claims apart by reading World alone.
     * WorkerLookup closes that gap with its own immediately-mutated
     * cache, refreshed from World once per tick before dispatch, while
     * still staging every claim into World via set() so the
     * WorkerCompletionSystem-visible state stays correct after commit.
     */
    class WorkerLookup final
    {
    public:
        /**
         * @brief Construct the lookup over its World and worker set.
         * @param world World the tracked entities live in.
         * @param workers Every Worker entity, in claim-priority order
         * (lowest index claimed first).
         */
        WorkerLookup(World &world, std::vector<Entity> workers);

        WorkerLookup(const WorkerLookup &) = delete;
        WorkerLookup(WorkerLookup &&) = delete;

        WorkerLookup &operator=(const WorkerLookup &) = delete;
        WorkerLookup &operator=(WorkerLookup &&) = delete;

        /**
         * @brief Replace the tracked entities with a restored pool's.
         * @param replacement Every Worker entity of the restored pool,
         * in claim-priority order, already committed into the World.
         *
         * What load_state calls once the old entities are destroyed:
         * the cache starts over as the constructor's would, and the
         * next tick's refresh() reads the restored statuses.
         */
        void reset(std::vector<Entity> replacement);

        /**
         * @brief Resync the idle/busy cache from World's committed
         * (front-buffer) Worker state.
         *
         * Call once per tick, before computing that tick's dispatch
         * budget and before any TaskJob claims a worker.
         */
        void refresh();

        /**
         * @brief Count workers the cache currently considers idle.
         * @return The number of idle workers as of the last refresh(),
         * minus any claims made since.
         */
        [[nodiscard]] std::size_t idleCount() const noexcept;

        /**
         * @brief Claim the lowest-index idle worker, if any.
         * @param durationTicks How many ticks the claimed worker stays
         * busy for.
         * @param taskId The claiming task's submission-script id,
         * recorded on the claimed Worker for status reporting.
         * @param label The claiming task's human-readable label,
         * recorded on the claimed Worker for status reporting;
         * truncated to antwika::ecs_commons::kNameMaxLength if longer.
         * @return True if a worker was claimed; false if none were
         * idle (should not happen when callers respect idleCount() as
         * a budget, but handled defensively rather than assumed away).
         */
        bool claimIdle(
            antwika::time::Tick durationTicks,
            std::uint64_t taskId,
            std::string_view label);

    private:
        World &world;
        std::vector<Entity> workers;
        std::vector<bool> idle;
    };

} // namespace antwika::task_worker
