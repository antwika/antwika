#pragma once

#include <cstddef>
#include <optional>

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/game/Building.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Footprint.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/Tuning.hpp"
#include "antwika/game/Walker.hpp"

namespace antwika::game
{

    using antwika::ecs::ISystem;
    using antwika::ecs::World;

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
     * @brief Find a slot in a building with nobody live in it.
     *
     * **The lowest free one, so two buildings that have sent the same
     * walkers in the same order hold them identically.** Nothing may
     * read a slot number as a role: they are places to put a handle and
     * nothing more.
     *
     * world.alive() is the authority rather than the handle being
     * kNullEntity, for the reason Building::walkers gives: an entry is a
     * cache, and ecs::EntityManager never reuses an index, so a stale
     * handle can only ever be dead.
     *
     * @param world Asked whether each handle is still alive.
     * @param building The building whose slots to look through.
     * @return The lowest free slot, or nothing when every slot holds a
     * live walker.
     */
    [[nodiscard]] std::optional<std::size_t> freeWalkerSlot(
        const World &world, const Building &building);

    /**
     * @brief Check whether a building already has this kind out.
     *
     * **The cadence sends one of its own kind at a time, and this is
     * that rule.** A slot is capacity rather than permission: a market
     * has two of them so that a buyer fetched by an errand and a seller
     * sent by the cadence can be out together, and reading a free slot
     * as leave to send another seller would double every building's
     * output the day the array grew.
     *
     * A handle that is alive but whose Walker component has not been
     * committed yet is one this building staged earlier in this very
     * tick, and counts: create() is immediate where add() is staged.
     *
     * @param world Asked about each handle.
     * @param building The building whose slots to look through.
     * @param kind The kind the cadence would send.
     * @return True when one of that kind is already out.
     */
    [[nodiscard]] bool hasWalkerOfKind(
        const World &world, const Building &building, WalkerKind kind);

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
     * **A building holds kMaxWalkersOut handles, and this system uses
     * one of them.** The rule it follows is hasWalkerOfKind(): the
     * cadence keeps one walker of the kind it sends out at a time, and
     * the remaining slots are room for the errands another system
     * sends. So growing the array does not, on its own, change what any
     * building emits.
     *
     * **The buildings are visited in ascending Cell rather than in
     * ecs::View order**, out of a std::map collected first, exactly as
     * LabourSystem and SupplySystem do. The walker cap is a limited
     * amount split between buildings, and a view iterates in an order
     * that is a property of the world's history rather than of the city
     * -- so at the cap the last free slots would otherwise go to
     * whichever buildings a restore happened to create first, and one
     * save loaded twice with its buildings in different orders could
     * disagree about which building sent the last walker.
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
         * @param tuning The spawn period and the walker cap; copied, so
         * no lifetime rule attaches to it.
         */
        SpawnSystem(const PathIndex &paths, Tuning tuning);

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
        Tuning tuning;
    };

} // namespace antwika::game
