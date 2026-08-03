#pragma once

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/time/Tick.hpp>

#include <antwika/ecs/Entity.hpp>

#include "antwika/game/Cell.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/Walker.hpp"

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
     * feeds -- including the bits it chooses an arm with, which come
     * from wanderRoll() off the tick, the cell and the walker's facing
     * rather than from a generator this would have to own.
     * A generator advanced once per decision would be state outside the
     * World that a save does not cover, which is exactly what keeps a
     * route out of a component too.
     *
     * Three properties come free from antwika::ecs and none needs
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
         * @brief Construct the system over the roads and the bounds.
         * @param paths Consulted for each walker's neighbours, and
         * searched for a route home; must outlive this system.
         * @param extent The bounds a route home is numbered over; see
         * stepTowards() for why it is stated rather than derived.
         */
        WalkerSystem(const PathIndex &paths, GridExtent extent);

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
         * **Once a walker's roaming budget is spent it either walks home
         * or it is gone**, and that one rule is what bounds the
         * population. Every awkward case collapses into its last arm: a
         * walker nobody sent, one whose building has since burned down,
         * one whose home has been walled off, one standing on a road
         * that was demolished under it. All four are answered by
         * destroying the walker rather than by four separate rules, and
         * none of them is an error.
         *
         * @param world The world to read walkers from and stage moves
         * into.
         * @param tick The tick being processed; unused.
         */
        void update(World &world, antwika::time::Tick tick) override;

    private:
        // Roaming and heading home are two whole rules.
        // Rather than two arms of one, so they are two functions.
        void travel(
            World &world,
            antwika::ecs::Entity entity,
            const Walker &walker,
            Cell at);

        void roam(
            World &world,
            antwika::ecs::Entity entity,
            const Walker &walker,
            Cell at,
            antwika::time::Tick tick);

        void headHome(
            World &world,
            antwika::ecs::Entity entity,
            const Walker &walker,
            Cell at);

        // And so is walking to somewhere in particular.
        // Which is the one arm that reads a walker's Errand.
        void runErrand(
            World &world,
            antwika::ecs::Entity entity,
            const Walker &walker,
            Cell at,
            antwika::ecs::Entity bound);

        const PathIndex &paths;
        GridExtent extent;
    };

} // namespace antwika::game
