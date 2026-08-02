#pragma once

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/game/GridExtent.hpp"
#include "antwika/game/PathIndex.hpp"

namespace antwika::game
{

    using antwika::ecs::ISystem;
    using antwika::ecs::World;

    /**
     * @brief Runs a market's two walkers: the buyer and the seller.
     *
     * The end of the chain. A buyer is sent to the nearest storehouse
     * holding the good, fetches a load and brings it back; a seller
     * takes a load out of what the market holds and hands it to the
     * houses it walks past.
     *
     * **This is what stops a market conjuring food out of nothing.**
     * SpawnSystem sends the seller on the market's cadence and gives it
     * the load its *kind* always carries, which was exact while nobody
     * had to supply a market. Here the seller is paid for out of the
     * market's own stock the tick after it appears -- before it has
     * walked anywhere, since a walker staged in one phase is invisible
     * to the delivery that ran in the same one -- and a market with an
     * empty shelf sends somebody out with an empty basket.
     *
     * The errand a seller is given names **nowhere**, which is what
     * tells BuildingSystem to hand its load to every house it passes
     * rather than to one named building, and is also the mark saying it
     * has already been paid for. A seller has no destination by nature:
     * where it goes is the whole point of it roaming.
     *
     * **Markets are walked in ascending Cell**, because two of them
     * buying from one storehouse in one tick is a limited amount split
     * between them, and the rule for those is an order somebody can
     * name. Their debits accumulate into one value per store and are
     * written once, exactly as BuildingSystem accumulates deliveries,
     * so the sum is the answer rather than the race.
     */
    class MarketSystem final : public ISystem
    {
    public:
        /**
         * @brief Construct the system over the roads its walkers use.
         * @param paths Consulted for each market's door and searched for
         * a route to each candidate store; must outlive this system.
         * @param extent The bounds that search is numbered over; see
         * nearestHolding() for why it is stated rather than derived.
         */
        MarketSystem(const PathIndex &paths, GridExtent extent);

        MarketSystem(const MarketSystem &) = delete;
        MarketSystem(MarketSystem &&) = delete;

        MarketSystem &operator=(const MarketSystem &) = delete;
        MarketSystem &operator=(MarketSystem &&) = delete;

        /**
         * @brief Pay every seller, send every buyer, and load the ones
         * that have arrived.
         * @param world Read for the markets and the stores, staged into
         * with the loads, the errands and the new buyers.
         * @param tick The tick being processed; unused.
         */
        void update(World &world, antwika::time::Tick tick) override;

    private:
        const PathIndex &paths;
        GridExtent extent;
    };

} // namespace antwika::game
