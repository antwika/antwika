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

        // What a market trades is read off its seller.
        // Rather than named a second time here.
        // Two statements of one thing is one to get wrong.
        static_assert(
            carriedResource(WalkerKind::MarketSeller)
            == fetchedFromStores(BuildingKind::Market));

        // Seeded from the last commit the first time it is touched.
        // So every change this tick accumulates onto one value.
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

        [[nodiscard]] bool besideBlock(
            Cell at, Cell origin, Footprint footprint)
        {
            for (std::size_t index = 0; index < kDirectionCount; ++index)
            {
                if (covers(
                        origin,
                        footprint,
                        step(at, static_cast<Direction>(index))))
                {
                    return true;
                }
            }

            return false;
        }
    } // namespace

    SupplySystem::SupplySystem(const PathIndex &paths, GridExtent extent)
        : paths(paths), extent(extent)
    {
    }

    void SupplySystem::update(World &world, antwika::time::Tick)
    {
        // Ascending Cell, out of a map rather than a view.
        // Two buyers fetching from one storehouse split what it holds.
        std::map<Cell, Entity> fetchers;

        for (const auto entity : world.view<Building, Cell>())
        {
            if (!fetchedFromStores(world.get<Building>(entity).kind)
                     .has_value())
            {
                continue;
            }

            fetchers.emplace(world.get<Cell>(entity), entity);
        }

        // Counted once rather than per building.
        // Every buyer staged this tick is one the cap has to see.
        std::size_t out = world.view<Walker>().size();

        Pending pending;

        for (const auto &[origin, entity] : fetchers)
        {
            const auto standing = world.get<Building>(entity);
            const auto kind = standing.kind;

            // What this kind cannot make, and therefore goes to get.
            const auto wanted = *fetchedFromStores(kind);
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
                    auto &held = shelf.stock[resourceIndex(wanted)];
                    const auto load = std::min(kWalkerLoad, held);
                    held -= load;

                    auto paid = walker;
                    paid.carried = load;
                    world.set<Walker>(walkerEntity, paid);

                    // An errand naming nowhere.
                    // Which is the mark saying it has been paid for.
                    // And has it hand the basket out as it roams.
                    world.add<Errand>(
                        walkerEntity,
                        Errand{
                            .destination = kNullEntity,
                            .carrying = wanted,
                            .leg = ErrandLeg::Outbound});
                    continue;
                }

                if (walker.kind != WalkerKind::MarketBuyer
                    || !world.has<Errand>(walkerEntity))
                {
                    continue;
                }

                const auto errand = world.get<Errand>(walkerEntity);

                // Credited here rather than in BuildingSystem.
                // That is the whole reason a buyer exists at all.
                // Both kinds that send one send walkers of their own.
                // So a cadence rewrites them in the walk phase.
                // Which would undo a delivery made there.
                // This phase is nobody else's -- see acceptsAt().
                if (errand.leg == ErrandLeg::Returning)
                {
                    if (walker.carried <= 0
                        || !besideBlock(
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

                if (!besideBlock(
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

                // Nothing left, or nowhere to put it, is no error.
                // A store emptied while it walked turns it round.
                // Rather than stranding it at the door.
                const auto taken = std::min(
                    {kWalkerLoad,
                     store.stock[resourceIndex(errand.carrying)],
                     room});
                store.stock[resourceIndex(errand.carrying)] -= taken;

                auto laden = walker;
                laden.carried = taken;
                world.set<Walker>(walkerEntity, laden);

                world.set<Errand>(
                    walkerEntity,
                    Errand{
                        .destination = errand.destination,
                        .carrying = errand.carrying,
                        .leg = ErrandLeg::Returning});
            }

            const auto slot = freeWalkerSlot(world, standing);

            if (shelf.stock[resourceIndex(wanted)] >= capacityOf(kind)
                || hasWalkerOfKind(
                    world, standing, WalkerKind::MarketBuyer)
                || !door.has_value() || !slot.has_value()
                || out >= kWalkerLimit)
            {
                continue;
            }

            const auto store = nearestHolding(
                world, *door, wanted, paths, extent);

            if (store == kNullEntity)
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
                    .destination = store,
                    .carrying = wanted,
                    .leg = ErrandLeg::Outbound});
            ++out;

            shelf.walkers[*slot] = buyer;
        }

        for (const auto &[entity, building] : pending)
        {
            world.set<Building>(entity, building);
        }
    }

} // namespace antwika::game
