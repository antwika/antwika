#include "antwika/game/HaulingSystem.hpp"

#include <algorithm>
#include <map>
#include <optional>

#include <antwika/ecs/Entity.hpp>

#include "antwika/game/Building.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Errand.hpp"
#include "antwika/game/ErrandRouting.hpp"
#include "antwika/game/Footprint.hpp"
#include "antwika/game/Production.hpp"
#include "antwika/game/SpawnSystem.hpp"
#include "antwika/game/Store.hpp"
#include "antwika/game/Walker.hpp"

namespace antwika::game
{

    namespace
    {
        using antwika::ecs::Entity;

        // Whether a routed cart has nothing left to do at its store.
        // Emptied, or heading for one that has since filled up.
        // The second is why a cart cannot stand at a full door for ever.
        [[nodiscard]] bool isSpent(
            const World &world, const Walker &walker, const Errand &errand)
        {
            if (errand.destination == antwika::ecs::kNullEntity)
            {
                return false;
            }

            if (walker.carried <= 0)
            {
                return true;
            }

            if (!world.has<Building>(errand.destination))
            {
                return false;
            }

            const auto kind =
                world.get<Building>(errand.destination).kind;

            return stockOf(world, errand.destination, errand.carrying)
                >= capacityOf(kind);
        }
    } // namespace

    HaulingSystem::HaulingSystem(const PathIndex &paths, GridExtent extent)
        : paths(paths), extent(extent)
    {
    }

    void HaulingSystem::update(World &world, antwika::time::Tick)
    {
        // Ascending Cell, out of a map rather than a view.
        // A producer's stock is a limited amount split among its carts.
        // BuildingIndex keeps two buildings off one origin cell.
        // So the cell alone is already a total order.
        std::map<Cell, Entity> producers;

        for (const auto entity : world.view<Building, Cell>())
        {
            if (!producedBy(world.get<Building>(entity).kind).has_value())
            {
                continue;
            }

            producers.emplace(world.get<Cell>(entity), entity);
        }

        for (const auto &[origin, entity] : producers)
        {
            auto building = world.get<Building>(entity);
            const auto resource = *producedBy(building.kind);
            bool loaded = false;

            for (const auto carrier : building.walkers)
            {
                if (!world.has<Walker>(carrier))
                {
                    continue;
                }

                const auto walker = world.get<Walker>(carrier);

                if (walker.kind != WalkerKind::CartPusher)
                {
                    continue;
                }

                if (world.has<Errand>(carrier))
                {
                    const auto errand = world.get<Errand>(carrier);

                    // Unbound rather than turned round.
                    // A cart done with its store is an ordinary walker.
                    // It roams out its budget and then heads home.
                    // What is left goes to the houses it passes.
                    // Rather than to the grave.
                    // Removing the errand would do the walking part.
                    // And lose the load, since the kind names none.
                    if (isSpent(world, walker, errand))
                    {
                        world.set<Errand>(
                            carrier,
                            Errand{
                                .destination = antwika::ecs::kNullEntity,
                                .carrying = errand.carrying,
                                .leg = errand.leg});
                    }

                    continue;
                }

                const auto load = std::min(
                    kWalkerLoad, building.stock[resourceIndex(resource)]);

                // Nothing to hand it, so it stays free to be loaded.
                // Rather than going out empty and having to die first.
                if (load <= 0)
                {
                    continue;
                }

                const auto door = spawnCellFor(
                    origin, footprintOf(building.kind), paths);

                // A producer with no road beside it reaches no store.
                // Which is the same answer as there being no store.
                const auto destination = door.has_value()
                    ? nearestAccepting(
                          world, *door, resource, paths, extent)
                    : antwika::ecs::kNullEntity;

                world.add<Errand>(
                    carrier,
                    Errand{
                        .destination = destination,
                        .carrying = resource,
                        .leg = ErrandLeg::Outbound});

                auto sent = walker;
                sent.carried = load;
                world.set<Walker>(carrier, sent);

                building.stock[resourceIndex(resource)] -= load;
                loaded = true;
            }

            // Written once, and only where something changed.
            // A needless whole-Building write undoes another.
            if (loaded)
            {
                world.set<Building>(entity, building);
            }
        }
    }

} // namespace antwika::game
