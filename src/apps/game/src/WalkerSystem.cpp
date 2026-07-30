#include "antwika/game/WalkerSystem.hpp"

#include <algorithm>

#include "antwika/game/Walking.hpp"

namespace antwika::game
{

    WalkerSystem::WalkerSystem(const PathIndex &paths) : paths(paths)
    {
    }

    void WalkerSystem::update(World &world, antwika::time::Tick)
    {
        std::map<Entity, Building> served;

        for (const auto entity : world.view<Walker, Cell>())
        {
            const auto at = world.get<Cell>(entity);
            auto walker = world.get<Walker>(entity);

            if (walker.stepsTaken >= kMaxWalkDistance)
            {
                // Where walking home goes.
                // A route back to where it came from would be worked out
                // here and followed instead, with the destroy() moved to
                // the tick it arrives -- one branch, in one place,
                // because nothing else in this file knows how far it has
                // come.
                world.destroy(entity);
                continue;
            }

            deliver(world, at, walker, served);

            const auto heading =
                nextFacing(walker.facing, paths.neighboursOf(at));

            if (heading.has_value())
            {
                walker.facing = *heading;
                ++walker.stepsTaken;
                world.set<Cell>(entity, step(at, *heading));
            }

            world.set<Walker>(entity, walker);
        }

        for (const auto &[entity, building] : served)
        {
            world.set<Building>(entity, building);
        }
    }

    void WalkerSystem::deliver(
        const World &world,
        Cell at,
        Walker &walker,
        std::map<Entity, Building> &served)
    {
        for (const auto entity : world.view<Building, Cell>())
        {
            if (!orthogonallyAdjacent(at, world.get<Cell>(entity)))
            {
                continue;
            }

            // Whatever an earlier walker left it this tick, if any.
            const auto found = served.find(entity);
            auto &building =
                found != served.end()
                    ? found->second
                    : served
                          .emplace(entity, world.get<Building>(entity))
                          .first->second;

            serve(walker, building);
        }
    }

    void serve(Walker &walker, Building &building)
    {
        const auto carries = carriedResource(walker.kind);

        if (!carries.has_value())
        {
            if (walker.kind == WalkerKind::Fireman)
            {
                building.fireRisk =
                    std::max(0, building.fireRisk - kRiskRelief);
                return;
            }

            building.collapseRisk =
                std::max(0, building.collapseRisk - kRiskRelief);
            return;
        }

        if (*carries != building.stock.resource)
        {
            return;
        }

        const auto room = building.stock.capacity - building.stock.held;
        const auto amount = std::min(walker.carried, room);

        if (amount <= 0)
        {
            return;
        }

        building.stock.held += amount;
        walker.carried -= amount;
    }

} // namespace antwika::game
