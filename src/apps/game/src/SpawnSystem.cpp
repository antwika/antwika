#include "antwika/game/SpawnSystem.hpp"

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>

#include <antwika/ecs/Entity.hpp>

#include "antwika/game/Building.hpp"
#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Direction.hpp"
#include "antwika/game/Footprint.hpp"
#include "antwika/game/LabourQuery.hpp"
#include "antwika/game/Walker.hpp"

namespace antwika::game
{

    SpawnSystem::SpawnSystem(const PathIndex &paths, GameConfig config)
        : paths(paths), config(config)
    {
    }

    std::optional<Cell> spawnCellFor(
        Cell origin, Footprint footprint, const PathIndex &paths)
    {
        std::optional<Cell> best;

        for (std::int32_t dy = 0; dy < footprint.height; ++dy)
        {
            for (std::int32_t dx = 0; dx < footprint.width; ++dx)
            {
                const Cell on{.x = origin.x + dx, .y = origin.y + dy};

                for (std::size_t index = 0; index < kDirectionCount;
                     ++index)
                {
                    const Cell beside =
                        step(on, static_cast<Direction>(index));

                    if (!paths.has(beside))
                    {
                        continue;
                    }

                    if (!best.has_value() || beside < *best)
                    {
                        best = beside;
                    }
                }
            }
        }

        return best;
    }

    std::optional<std::size_t> freeWalkerSlot(
        const World &world, const Building &building)
    {
        for (std::size_t slot = 0; slot < kMaxWalkersOut; ++slot)
        {
            if (!world.alive(building.walkers[slot]))
            {
                return slot;
            }
        }

        return std::nullopt;
    }

    bool hasWalkerOfKind(
        const World &world, const Building &building, WalkerKind kind)
    {
        for (const auto walker : building.walkers)
        {
            if (!world.alive(walker))
            {
                continue;
            }

            if (!world.has<Walker>(walker)
                || world.get<Walker>(walker).kind == kind)
            {
                return true;
            }
        }

        return false;
    }

    void SpawnSystem::update(World &world, antwika::time::Tick)
    {
        using antwika::ecs::Entity;

        std::size_t out = world.view<Walker>().size();

        std::map<Cell, Entity> senders;

        for (const auto entity : world.view<Building, Cell>())
        {
            if (!walkerSentBy(world.get<Building>(entity).kind)
                     .has_value())
            {
                continue;
            }

            senders.emplace(world.get<Cell>(entity), entity);
        }

        for (const auto &[at, entity] : senders)
        {
            const auto building = world.get<Building>(entity);
            const auto sends = *walkerSentBy(building.kind);

            if (hasWalkerOfKind(world, building, sends))
            {
                continue;
            }

            const auto period = workedPeriod(
                config.spawnPeriodTicks, staffingOf(world, entity));

            if (!period.has_value())
            {
                continue;
            }

            if (building.ticksUntilSpawn > 0)
            {
                auto waiting = building;
                waiting.ticksUntilSpawn = building.ticksUntilSpawn - 1;
                world.set<Building>(entity, waiting);
                continue;
            }

            const auto onto =
                spawnCellFor(at, footprintOf(building.kind), paths);
            const auto slot = freeWalkerSlot(world, building);

            if (!onto.has_value() || !slot.has_value()
                || out >= config.walkerLimit)
            {
                continue;
            }

            const auto walker = world.create();
            world.add<Cell>(walker, *onto);
            world.add<Walker>(
                walker, Walker{.kind = sends, .carried = 0, .home = entity});
            ++out;

            auto sent = building;
            sent.ticksUntilSpawn = *period - 1;
            sent.walkers[*slot] = walker;
            world.set<Building>(entity, sent);
        }
    }

}
