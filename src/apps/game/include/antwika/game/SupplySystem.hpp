#pragma once

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/game/GridExtent.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/Tuning.hpp"

namespace antwika::game
{

    using antwika::ecs::ISystem;
    using antwika::ecs::World;

    /**
     * @brief Sends a buyer out for what a building cannot make, and
     * runs a market's seller besides.
     *
     * The end of the chain, and the one route by which goods reach a
     * building that sends walkers of its own.
     * A buyer goes to the nearest storehouse holding what its workplace
     * needs, fetches a load and brings it back; the load is credited
     * here, in this system's own phase, because acceptsAt() explains at
     * length why a cart may not fill such a building at all.
     *
     * **Which kinds go and get something is fetchedFromStores(), and it
     * is two.** A market fetches the food its seller hands out. A
     * workshop fetches the clay it fires into pottery, which is what
     * turns pottery from a resource with no way into the building that
     * would use it into something a running city makes.
     * One code path and a table rather than a system each: the two want
     * exactly the same walk, the same crediting and the same order over
     * the stores they compete for, and a copy of it would be a second
     * place for that order to drift.
     *
     * **This is also what stops a market conjuring food out of
     * nothing.** SpawnSystem sends the seller on the market's cadence
     * and gives it the load its *kind* always carries, which was exact
     * while nobody had to supply a market. Here the seller is paid for
     * out of the market's own stock the tick after it appears -- before
     * it has walked anywhere, since a walker staged in one phase is
     * invisible to the delivery that ran in the same one -- and a
     * market with an empty shelf sends somebody out with an empty
     * basket.
     *
     * The errand a seller is given names **nowhere**, which is what
     * tells BuildingSystem to hand its load to every house it passes
     * rather than to one named building, and is also the mark saying it
     * has already been paid for. A seller has no destination by nature:
     * where it goes is the whole point of it roaming.
     *
     * **The buyers are walked in ascending Cell**, because two of them
     * fetching from one storehouse in one tick is a limited amount
     * split between them, and the rule for those is an order somebody
     * can name. Their debits accumulate into one value per store and
     * are written once, exactly as BuildingSystem accumulates
     * deliveries, so the sum is the answer rather than the race.
     *
     * **It has a phase to itself, and a workshop is why.** It used to
     * share "haul" with HaulingSystem, which was safe while a cart was
     * loaded out of a producer, a buyer was loaded out of a storehouse
     * and no building was both. A workshop is both: a cart takes its
     * pottery away and a buyer brings its clay in, so the two would
     * write one Building in one phase and the later write would
     * silently undo the earlier.
     */
    class SupplySystem final : public ISystem
    {
    public:
        /**
         * @brief Construct the system over the roads its walkers use.
         * @param paths Consulted for each market's door and searched for
         * a route to each candidate store; must outlive this system.
         * @param extent The bounds that search is numbered over; see
         * nearestHolding() for why it is stated rather than derived.
         * @param tuning The walker cap; copied, so no lifetime rule
         * attaches to it.
         */
        SupplySystem(
            const PathIndex &paths, GridExtent extent, Tuning tuning);

        SupplySystem(const SupplySystem &) = delete;
        SupplySystem(SupplySystem &&) = delete;

        SupplySystem &operator=(const SupplySystem &) = delete;
        SupplySystem &operator=(SupplySystem &&) = delete;

        /**
         * @brief Pay every seller, send every buyer, and load the ones
         * that have arrived.
         * @param world Read for the fetching buildings and the stores,
         * staged into with the loads, the errands and the new buyers.
         * @param tick The tick being processed; unused.
         */
        void update(World &world, antwika::time::Tick tick) override;

    private:
        const PathIndex &paths;
        GridExtent extent;
        Tuning tuning;
    };

} // namespace antwika::game
