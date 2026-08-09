#include "antwika/game/SupplySystem.hpp"

#include <algorithm>
#include <cstddef>
#include <map>

#include <antwika/ecs/Entity.hpp>

#include "antwika/game/Building.hpp"
#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Direction.hpp"
#include "antwika/game/Errand.hpp"
#include "antwika/game/ErrandRouting.hpp"
#include "antwika/game/Footprint.hpp"
#include "antwika/game/SpawnSystem.hpp"
#include "antwika/game/Store.hpp"
#include "antwika/game/Walker.hpp"

namespace antwika::game
{

    namespace
    {
        using antwika::ecs::Entity;
        using antwika::ecs::kNullEntity;

        using Pending = std::map<Entity, Building>;

        struct Purchase final
        {
            Entity store;

            Resource resource;
        };

        [[nodiscard]] Building &touch(
            const World &world, Pending &pending, Entity entity)
        {
            const auto found = pending.find(entity);

            if (found != pending.end())
            {
                return found->second;
            }

            return pending.emplace(entity, world.get<Building>(entity))
                .first->second;
        }

        [[nodiscard]] Resource nextSold(const Building &building)
        {
            auto sold = building.selling;

            for (std::size_t step = 1; step <= kResourceCount; ++step)
            {
                const auto turn =
                    (resourceIndex(building.selling) + step) % kResourceCount;

                const auto candidate = kResources[turn];

                if (fetchesFromStores(building.kind, candidate)
                    && building.stock[resourceIndex(candidate)] > 0)
                {
                    sold = candidate;
                    break;
                }
            }

            return sold;
        }

        [[nodiscard]] std::int32_t committed(
            const World &world,
            const Building &building,
            Resource resource)
        {
            auto held = building.stock[resourceIndex(resource)];

            for (const auto carrier : building.walkers)
            {
                if (!world.has<Walker>(carrier)
                    || !world.has<Errand>(carrier))
                {
                    continue;
                }

                if (world.get<Errand>(carrier).carrying == resource)
                {
                    held += world.get<Walker>(carrier).carried;
                }
            }

            return held;
        }

        [[nodiscard]] Purchase shoppingFor(
            const World &world,
            const Building &building,
            Cell door,
            const PathIndex &paths,
            GridExtent extent)
        {
            Purchase best{
                .store = kNullEntity, .resource = kResources.front()};

            std::int32_t least = 0;

            for (const auto resource : kResources)
            {
                if (!fetchesFromStores(building.kind, resource)
                    || building.stock[resourceIndex(resource)]
                        >= capacityOf(building.kind))
                {
                    continue;
                }

                const auto held = committed(world, building, resource);

                if (best.store != kNullEntity && held >= least)
                {
                    continue;
                }

                const auto store =
                    nearestHolding(world, door, resource, paths, extent);

                if (store == kNullEntity)
                {
                    continue;
                }

                best.store = store;
                best.resource = resource;
                least = held;
            }

            return best;
        }

    }

    SupplySystem::SupplySystem(
        const PathIndex &paths, GridExtent extent, GameConfig config)
        : paths(paths), extent(extent), config(config)
    {
    }

    void SupplySystem::update(World &world, antwika::time::Tick)
    {
        std::map<Cell, Entity> fetchers;

        for (const auto entity : world.view<Building, Cell>())
        {
            if (!fetchesFromStores(world.get<Building>(entity).kind))
            {
                continue;
            }

            fetchers.emplace(world.get<Cell>(entity), entity);
        }

        std::size_t out = world.view<Walker>().size();

        Pending pending;

        for (const auto &[origin, entity] : fetchers)
        {
            const auto standing = world.get<Building>(entity);
            const auto kind = standing.kind;

            const auto footprint = footprintOf(kind);
            const auto door = spawnCellFor(origin, footprint, paths);
            auto &shelf = touch(world, pending, entity);

            for (const auto walkerEntity : standing.walkers)
            {
                if (!world.has<Walker>(walkerEntity))
                {
                    continue;
                }

                const auto walker = world.get<Walker>(walkerEntity);

                if (walker.kind == WalkerKind::MarketSeller
                    && !world.has<Errand>(walkerEntity))
                {
                    const auto sold = nextSold(shelf);
                    auto &held = shelf.stock[resourceIndex(sold)];
                    const auto load = std::min(kWalkerLoad, held);
                    held -= load;
                    shelf.selling = sold;

                    auto paid = walker;
                    paid.carried = load;
                    world.set<Walker>(walkerEntity, paid);

                    world.add<Errand>(
                        walkerEntity,
                        Errand{
                            .destination = kNullEntity,
                            .carrying = sold,
                            .leg = ErrandLeg::Outbound});
                    continue;
                }

                if (walker.kind != WalkerKind::MarketBuyer
                    || !world.has<Errand>(walkerEntity))
                {
                    continue;
                }

                const auto errand = world.get<Errand>(walkerEntity);
                const auto wanted = errand.carrying;

                if (errand.leg == ErrandLeg::Returning)
                {
                    if (walker.carried <= 0
                        || !beside(
                            world.get<Cell>(walkerEntity),
                            origin,
                            footprint))
                    {
                        continue;
                    }

                    auto &held = shelf.stock[resourceIndex(wanted)];
                    const auto given =
                        std::min(walker.carried, capacityOf(kind) - held);
                    held += given;

                    auto emptied = walker;
                    emptied.carried -= given;
                    world.set<Walker>(walkerEntity, emptied);
                    continue;
                }

                if (!world.has<Cell>(errand.destination))
                {
                    continue;
                }

                if (!beside(
                        world.get<Cell>(walkerEntity),
                        world.get<Cell>(errand.destination),
                        footprintOf(
                            world.get<Building>(errand.destination)
                                .kind)))
                {
                    continue;
                }

                auto &store = touch(world, pending, errand.destination);
                const auto room =
                    capacityOf(kind) - shelf.stock[resourceIndex(wanted)];

                const auto taken = std::min(
                    {kWalkerLoad, store.stock[resourceIndex(wanted)], room});
                store.stock[resourceIndex(wanted)] -= taken;

                auto laden = walker;
                laden.carried = taken;
                world.set<Walker>(walkerEntity, laden);

                world.set<Errand>(
                    walkerEntity,
                    Errand{
                        .destination = errand.destination,
                        .carrying = wanted,
                        .leg = ErrandLeg::Returning});
            }

            const auto slot = freeWalkerSlot(world, standing);

            if (hasWalkerOfKind(world, standing, WalkerKind::MarketBuyer)
                || !door.has_value() || !slot.has_value()
                || out >= config.walkerLimit)
            {
                continue;
            }

            const auto bought =
                shoppingFor(world, shelf, *door, paths, extent);

            if (bought.store == kNullEntity)
            {
                continue;
            }

            const auto buyer = world.create();
            world.add<Cell>(buyer, *door);
            world.add<Walker>(
                buyer,
                Walker{
                    .kind = WalkerKind::MarketBuyer,
                    .carried = 0,
                    .home = entity});
            world.add<Errand>(
                buyer,
                Errand{
                    .destination = bought.store,
                    .carrying = bought.resource,
                    .leg = ErrandLeg::Outbound});
            ++out;

            shelf.walkers[*slot] = buyer;
        }

        for (const auto &[entity, building] : pending)
        {
            world.set<Building>(entity, building);
        }
    }

}
