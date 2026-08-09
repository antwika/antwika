#include "antwika/game/ErrandRouting.hpp"

#include <cstdint>
#include <map>
#include <optional>

#include "antwika/game/Building.hpp"
#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Footprint.hpp"
#include "antwika/game/Homing.hpp"
#include "antwika/game/Store.hpp"

namespace antwika::game
{

    namespace
    {
        using antwika::ecs::Entity;
        using antwika::ecs::kNullEntity;
        using antwika::ecs::World;

        using Suitable = bool (*)(const Building &, Resource);

        [[nodiscard]] bool hasRoomFor(
            const Building &building, Resource resource)
        {
            return acceptsAt(building.kind, resource)
                && building.stock[resourceIndex(resource)]
                       < capacityOf(building.kind);
        }

        [[nodiscard]] bool holdsSome(
            const Building &building, Resource resource)
        {
            return suppliesMarkets(building.kind)
                && building.stock[resourceIndex(resource)] > 0;
        }

        [[nodiscard]] Entity nearest(
            const World &world,
            Cell from,
            Resource resource,
            const PathIndex &paths,
            GridExtent extent,
            Suitable suitable)
        {
            std::map<Cell, Entity> candidates;

            for (const auto entity : world.view<Building, Cell>())
            {
                if (!suitable(world.get<Building>(entity), resource))
                {
                    continue;
                }

                candidates.emplace(world.get<Cell>(entity), entity);
            }

            Entity best = kNullEntity;
            std::optional<std::int64_t> shortest;

            for (const auto &[at, entity] : candidates)
            {
                const auto cost = routeCost(
                    from,
                    at,
                    footprintOf(world.get<Building>(entity).kind),
                    paths,
                    extent);

                if (!cost.has_value())
                {
                    continue;
                }

                if (!shortest.has_value() || *cost < *shortest)
                {
                    shortest = cost;
                    best = entity;
                }
            }

            return best;
        }
    }

    antwika::ecs::Entity nearestAccepting(
        const antwika::ecs::World &world,
        Cell from,
        Resource resource,
        const PathIndex &paths,
        GridExtent extent)
    {
        return nearest(world, from, resource, paths, extent, hasRoomFor);
    }

    antwika::ecs::Entity nearestHolding(
        const antwika::ecs::World &world,
        Cell from,
        Resource resource,
        const PathIndex &paths,
        GridExtent extent)
    {
        return nearest(world, from, resource, paths, extent, holdsSome);
    }

}
