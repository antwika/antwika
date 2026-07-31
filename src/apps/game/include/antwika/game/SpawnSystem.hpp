#pragma once

#include <cstddef>
#include <optional>

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/game/Cell.hpp"
#include "antwika/game/Footprint.hpp"
#include "antwika/game/PathIndex.hpp"

namespace antwika::game
{

    using antwika::ecs::ISystem;
    using antwika::ecs::World;

    /**
     * @brief How many walkers a session may have out at once.
     *
     * A run left going has an unbounded number of buildings on it and no
     * reason for any walker to leave, so without a cap the population and
     * the per-tick work behind it grow for ever. Sixty-four is well past
     * what a 24x24 grid reads as busy and far below what a tick costs
     * anything to walk.
     *
     * A building at the cap *holds* its countdown at zero rather than
     * resetting it, so the moment somebody wanders off the end of the
     * world -- or the cap is raised -- the next one leaves at once.
     */
    inline constexpr std::size_t kWalkerLimit = 64;

    /**
     * @brief Find the path cell a building would send somebody out onto.
     *
     * The lowest of the four neighbours that has one, in Cell's own
     * ordering. Deterministic by construction rather than by luck: a
     * building with two roads beside it must pick the same one every run,
     * and picking "the first one found" while walking an unordered
     * container would not.
     *
     * @param at The building's cell.
     * @param paths Which cells have roads.
     * @return The cell to spawn onto, or nothing when no neighbour has a
     * road.
     */
    [[nodiscard]] std::optional<Cell> spawnCellFor(
        Cell origin, Footprint footprint, const PathIndex &paths);

    /**
     * @brief Sends a walker out of every building, on its own cadence.
     *
     * The counterpart to WalkerSystem, and the same shape: the interval
     * lives in each building's own component and counts down, rather than
     * being a modulus on the tick number, so two houses placed a tick
     * apart keep their own rhythm instead of marching together for ever.
     *
     * **A building that cannot spawn holds its countdown at zero rather
     * than resetting it.** That is the difference between a house being
     * ready and waiting for a road, and a house that owes a queue of
     * walkers to whoever finally builds one: a countdown that cannot go
     * below zero cannot accumulate a debt, so laying a road beside a long
     * neglected house releases one walker rather than twenty.
     *
     * Nothing here is a persisted event, for the reason GridSink gives
     * about placing a tile: a spawn follows from the click that placed
     * the building, and writing it alongside that click would spawn twice
     * on replay. It is a pure function of the tick and the state, so a
     * replay reproduces every walker it produced.
     */
    class SpawnSystem final : public ISystem
    {
    public:
        /**
         * @brief Construct the system over the roads it spawns onto.
         * @param paths Consulted for each building's neighbours; must
         * outlive this system.
         */
        explicit SpawnSystem(const PathIndex &paths);

        SpawnSystem(const SpawnSystem &) = delete;
        SpawnSystem(SpawnSystem &&) = delete;

        SpawnSystem &operator=(const SpawnSystem &) = delete;
        SpawnSystem &operator=(SpawnSystem &&) = delete;

        /**
         * @brief Count every building down, and spawn the ones that are
         * due.
         * @param world Read for the buildings, staged into with the new
         * walkers.
         * @param tick The tick being run; unused, deliberately -- see the
         * class comment on why the cadence is not a modulus on it.
         */
        void update(World &world, antwika::time::Tick tick) override;

    private:
        const PathIndex &paths;
    };

} // namespace antwika::game
