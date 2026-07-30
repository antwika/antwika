#include "antwika/game/WalkerSystem.hpp"

#include <algorithm>

#include <antwika/pathfinding/AStar.hpp>
#include <antwika/pathfinding/SearchResult.hpp>

#include "antwika/game/RoadGraph.hpp"
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
                if (!walkHome(world, entity, at, walker))
                {
                    world.destroy(entity);
                }

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

    bool WalkerSystem::walkHome(
        World &world, Entity entity, Cell at, const Walker &walker) const
    {
        if (at == walker.origin)
        {
            return false;
        }

        const RoadGraph roads(paths);
        const auto route = antwika::pathfinding::findPath(
            roads, nodeFor(at), nodeFor(walker.origin));

        if (route.outcome == antwika::pathfinding::SearchOutcome::NoPath)
        {
            return false;
        }

        // The route runs start-to-goal inclusive.
        // The start is not the goal here.
        // So there is always a second node to step onto.
        const auto onto = cellFor(route.nodes[1]);

        auto heading = walker;
        heading.facing = headingTo(at, onto);
        ++heading.stepsTaken;

        world.set<Cell>(entity, onto);
        world.set<Walker>(entity, heading);

        return true;
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
