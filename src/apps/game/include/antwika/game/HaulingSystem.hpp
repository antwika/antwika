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
     * @brief Loads a producer's cart and points it at a store.
     *
     * The first walker in this application with a *destination* rather
     * than a preference order, and the second half of what makes a farm
     * feed anybody.
     *
     * **A cart with nowhere to unload is still loaded**, with an errand
     * naming nowhere, and it takes the load round with it instead --
     * handing it to whatever it walks past exactly as the food walker of
     * the version-2 vocabulary did. That is not a fallback bolted on: it
     * is what keeps a city migrated from such a file fed while it has no
     * storehouse in it yet, and it is the reason a null destination is an
     * ordinary state of Errand rather than a missing one.
     *
     * A cart that has emptied, or whose store filled up while it walked,
     * is turned round rather than left standing: it walks home with
     * whatever is left and hands that back, so nothing is destroyed by a
     * store that filled at the wrong moment and no cart can occupy its
     * building's slot for ever.
     *
     * **This runs in a phase after the production one**, which is what
     * keeps taking a cart-load off a farm and adding a batch to it from
     * being two whole-Building writes that undo each other; see
     * ProductionSystem.
     */
    class HaulingSystem final : public ISystem
    {
    public:
        /**
         * @brief Construct the system over the roads a cart may use.
         * @param paths Searched for a route to each candidate store;
         * must outlive this system.
         * @param extent The bounds that search is numbered over; see
         * nearestAccepting() for why it is stated rather than derived.
         */
        HaulingSystem(const PathIndex &paths, GridExtent extent);

        HaulingSystem(const HaulingSystem &) = delete;
        HaulingSystem(HaulingSystem &&) = delete;

        HaulingSystem &operator=(const HaulingSystem &) = delete;
        HaulingSystem &operator=(HaulingSystem &&) = delete;

        /**
         * @brief Load every idle cart, and turn round every spent one.
         *
         * **Producers are walked in ascending Cell, out of a std::map
         * rather than a view**, because what is split here is a
         * building's own stock among the carts it has out -- a limited
         * amount, and the rule for those is an order somebody can name.
         *
         * @param world Read for the producers and their carts, staged
         * into with the loads and the errands.
         * @param tick The tick being processed; unused.
         */
        void update(World &world, antwika::time::Tick tick) override;

    private:
        const PathIndex &paths;
        GridExtent extent;
    };

} // namespace antwika::game
