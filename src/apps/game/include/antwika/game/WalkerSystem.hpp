#pragma once

#include <map>

#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/game/Building.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/Walker.hpp"

namespace antwika::game
{

    using antwika::ecs::Entity;
    using antwika::ecs::ISystem;
    using antwika::ecs::World;

    /**
     * @brief Advances every walker one cell per tick, along the paths,
     * serving what it passes on the way.
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
     *
     * What does need code here is that two walkers may serve one building
     * in one tick: a World::set() is a write and not an addition, so the
     * second would undo the first. Every top-up this tick is therefore
     * gathered into one std::map first -- ordered by entity, so what it
     * stages never depends on a hash -- and staged once at the end.
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
         * @brief Serve, then move, every walker.
         *
         * A walker whose cell has no path neighbour at all stays put and
         * keeps its facing -- it was dropped on a one-tile path and has
         * nowhere to go, including backwards. It still serves what is
         * beside it, since standing still is not being idle.
         *
         * A walker that has already taken kMaxWalkDistance steps is gone
         * before it serves anything: it is out of the world on the tick
         * it would have taken its next step.
         *
         * @param world The world to read walkers from and stage moves,
         * deliveries and despawns into.
         * @param tick The tick being processed; unused.
         */
        void update(World &world, antwika::time::Tick tick) override;

    private:
        static void deliver(
            const World &world,
            Cell at,
            Walker &walker,
            std::map<Entity, Building> &served);

        const PathIndex &paths;
    };

    /**
     * @brief Hand one building whatever one walker has for it.
     *
     * A free function rather than a method, because it is the whole of
     * what a walker does to a building and is worth asserting on its own,
     * without a world to put the two into.
     *
     * A food walker fills only what stocks food, and a water walker only
     * what stocks water; a fireman and an architect carry nothing and
     * take risk off instead. Nothing here can push a building past its
     * capacity, and nothing can push a risk below zero.
     *
     * @param walker The walker passing by, drawn down by what it hands
     * over.
     * @param building The building beside it, topped up or relieved.
     */
    void serve(Walker &walker, Building &building);

} // namespace antwika::game
