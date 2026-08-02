#pragma once

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/time/Tick.hpp>

namespace antwika::game
{

    using antwika::ecs::ISystem;
    using antwika::ecs::World;

    /**
     * @brief Turns a producer's countdown into goods in its own store.
     *
     * The first half of the chain the genre is about: a farm and a clay
     * pit make something out of the ground, and a workshop makes
     * something out of what a cart brought it.
     *
     * **The countdown lives in each building's own Production
     * component**, for the reason Building's three countdowns do -- two
     * workshops put up a tick apart would otherwise finish in lockstep
     * for ever -- and a producer that has none is given one rather than
     * skipped, so a building placed now and a building read out of a
     * file written before this component existed behave identically.
     *
     * **A producer that cannot work holds its countdown at zero rather
     * than resetting it**, which is SpawnSystem's rule and it is the
     * same difference: a workshop that has been out of clay for a
     * thousand ticks owes nobody a thousand batches the moment a cart
     * arrives.
     *
     * Nothing here is a persisted event, for the reason a spawn is not
     * one: a batch follows from the click that placed the building, and
     * writing it beside that click would produce it twice on replay.
     *
     * **This system has a phase to itself**, ahead of the hauling one.
     * A phase is where the World's buffers swap, and two systems in one
     * phase both read the value as of the last swap -- so a system that
     * added a batch to a farm's stock and a system that took a cart-load
     * off it would each write a whole Building back and the later one
     * would silently undo the earlier. The commit between them is what
     * makes the two arithmetic rather than a race.
     */
    class ProductionSystem final : public ISystem
    {
    public:
        ProductionSystem() = default;

        ProductionSystem(const ProductionSystem &) = delete;
        ProductionSystem(ProductionSystem &&) = delete;

        ProductionSystem &operator=(const ProductionSystem &) = delete;
        ProductionSystem &operator=(ProductionSystem &&) = delete;

        /**
         * @brief Count every producer down, and finish the ones that are
         * due.
         *
         * Iterated straight off a view rather than out of an ordered
         * map, because the effect is **independent per entity**: a
         * building's batch is made out of its own stock and nothing is
         * shared between two of them, so no order over them is
         * observable.
         *
         * @param world Read for the producers, staged into with their
         * output.
         * @param tick The tick being processed; unused, deliberately --
         * see the class comment on why the cadence is not a modulus on
         * it.
         */
        void update(World &world, antwika::time::Tick tick) override;
    };

} // namespace antwika::game
