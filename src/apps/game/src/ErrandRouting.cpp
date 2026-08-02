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

        // A function pointer rather than a template or a std::function.
        // There are exactly two of these and both are named below.
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

        // **Ascending Cell, out of a std::map rather than a view.**
        // Which store is chosen decides where a limited load goes.
        // So it may not depend on which storage happens to be smallest.
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

                // Strictly shorter, so a tie keeps the lower Cell.
                // Which is the one the map handed over first.
                if (!shortest.has_value() || *cost < *shortest)
                {
                    shortest = cost;
                    best = entity;
                }
            }

            return best;
        }
    } // namespace

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

} // namespace antwika::game
