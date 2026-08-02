#include "antwika/game/MarketSystem.hpp"

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

        // Read off the seller rather than named a second time here.
        // Two statements of what a market trades is one to get wrong.
        constexpr Resource kMarketGood = Resource::Food;
        static_assert(
            carriedResource(WalkerKind::MarketSeller) == kMarketGood);

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

    MarketSystem::MarketSystem(const PathIndex &paths, GridExtent extent)
        : paths(paths), extent(extent)
    {
    }

    void MarketSystem::update(World &world, antwika::time::Tick)
    {
        // Ascending Cell, out of a map rather than a view.
        // Two markets buying from one storehouse split what it holds.
        std::map<Cell, Entity> markets;

        for (const auto entity : world.view<Building, Cell>())
        {
            if (world.get<Building>(entity).kind != BuildingKind::Market)
            {
                continue;
            }

            markets.emplace(world.get<Cell>(entity), entity);
        }

        // Counted once rather than per market.
        // Every buyer staged this tick is one the cap has to see.
        std::size_t out = world.view<Walker>().size();

        Pending pending;

        for (const auto &[origin, entity] : markets)
        {
            const auto footprint = footprintOf(BuildingKind::Market);
            const auto door = spawnCellFor(origin, footprint, paths);
            const auto standing = world.get<Building>(entity);
            auto &market = touch(world, pending, entity);

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
                    auto &held = market.stock[resourceIndex(kMarketGood)];
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
                            .carrying = kMarketGood,
                            .leg = ErrandLeg::Outbound});
                    continue;
                }

                if (walker.kind != WalkerKind::MarketBuyer
                    || !world.has<Errand>(walkerEntity))
                {
                    continue;
                }

                const auto errand = world.get<Errand>(walkerEntity);

                // The market is credited here, not in BuildingSystem.
                // That is the whole reason a buyer exists at all.
                // A market sends walkers.
                // So its cadence rewrites it in the walk phase.
                // Which would undo a delivery made there.
                // This phase is the market's own -- see acceptsAt().
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

                    auto &held = market.stock[resourceIndex(kMarketGood)];
                    const auto given = std::min(
                        walker.carried,
                        capacityOf(BuildingKind::Market) - held);
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
                const auto room = capacityOf(BuildingKind::Market)
                    - market.stock[resourceIndex(kMarketGood)];

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

            if (market.stock[resourceIndex(kMarketGood)]
                    >= capacityOf(BuildingKind::Market)
                || hasWalkerOfKind(
                    world, standing, WalkerKind::MarketBuyer)
                || !door.has_value() || !slot.has_value()
                || out >= kWalkerLimit)
            {
                continue;
            }

            const auto store = nearestHolding(
                world, *door, kMarketGood, paths, extent);

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
                    .carrying = kMarketGood,
                    .leg = ErrandLeg::Outbound});
            ++out;

            market.walkers[*slot] = buyer;
        }

        for (const auto &[entity, building] : pending)
        {
            world.set<Building>(entity, building);
        }
    }

} // namespace antwika::game
