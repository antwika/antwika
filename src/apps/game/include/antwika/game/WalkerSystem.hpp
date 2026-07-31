#pragma once

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/game/PathIndex.hpp"

namespace antwika::game
{

    using antwika::ecs::ISystem;
    using antwika::ecs::World;

    /**
     * @brief Advances every walker along the paths, a cell at a time.
     *
     * A step takes kTicksPerStep ticks: on the ticks in between, the
     * walker's own countdown is all that moves. The countdown is per
     * walker rather than a modulus on the tick number, so walkers dropped
     * on different ticks do not march in lockstep.
     *
     * The rule itself is nextFacing(), which is a pure function this only
     * feeds. Three properties come free from antwika::ecs and none needs
     * code here:
     *
     * Walkers cannot see each other's moves within a tick, because World
     * double-buffers and only swaps at commit(). Two walkers meeting
     * head-on pass through each other rather than one reacting to the
     * other's new position, and the result never depends on which was
     * created first.
     *
     * Iteration order is stable, because View orders by ComponentStorage's
     * insertion order.
     *
     * Two walkers may share a cell. Nothing in the requirements forbids
     * it, and inventing a rule to avoid it would be inventing a
     * requirement.
     */
    class WalkerSystem final : public ISystem
    {
    public:
        /**
         * @brief Construct the system over the paths it walks.
         * @param paths Consulted for each walker's neighbours; must
         * outlive this system.
         */
        explicit WalkerSystem(const PathIndex &paths);

        WalkerSystem(const WalkerSystem &) = delete;
        WalkerSystem(WalkerSystem &&) = delete;

        WalkerSystem &operator=(const WalkerSystem &) = delete;
        WalkerSystem &operator=(WalkerSystem &&) = delete;

        /**
         * @brief Move each walker one cell, or leave it where it is.
         *
         * A walker part-way through a step stays put and counts down.
         *
         * A walker whose cell has no path neighbour at all stays put and
         * keeps its facing -- it was dropped on a one-tile path and has
         * nowhere to go, including backwards. Its countdown is already
         * spent, so it looks for a way on every tick rather than every
         * other one.
         *
         * @param world The world to read walkers from and stage moves
         * into.
         * @param tick The tick being processed; unused.
         */
        void update(World &world, antwika::time::Tick tick) override;

    private:
        const PathIndex &paths;
    };

} // namespace antwika::game
