#include "antwika/game/SpawnSystem.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>

#include "antwika/game/Building.hpp"
#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Direction.hpp"
#include "antwika/game/Footprint.hpp"
#include "antwika/game/LabourQuery.hpp"
#include "antwika/game/Walker.hpp"

namespace antwika::game
{

    SpawnSystem::SpawnSystem(const PathIndex &paths) : paths(paths)
    {
    }

    std::optional<Cell> spawnCellFor(
        Cell origin, Footprint footprint, const PathIndex &paths)
    {
        std::optional<Cell> best;

        // Every cell of the block, and every road beside one.
        // A block's own cells are skipped by paths.has().
        // Nothing can be both a road and a building.
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

                    // The lowest, in Cell's own ordering.
                    // Two roads beside one building pick the same one.
                    // Whichever order the walk happened to find them.
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

            // A handle staged this tick has no component yet.
            // It is still one this building has out.
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
        // Counted once rather than per building.
        // Every walker staged this tick is one the cap has to see.
        std::size_t out = world.view<Walker>().size();

        for (const auto entity : world.view<Building, Cell>())
        {
            const auto building = world.get<Building>(entity);
            const auto sends = walkerSentBy(building.kind);

            if (!sends.has_value())
            {
                continue;
            }

            // One of this cadence's own kind out at a time.
            // A free slot is room for an errand, not leave to send two.
            //
            // A walker destroyed this tick reads alive until commit.
            // So its building is free from the next tick, not this one.
            // Which is why the two never overlap.
            if (hasWalkerOfKind(world, building, *sends))
            {
                continue;
            }

            // Stretched over however few people turned up for work.
            // Half a complement is half as often.
            // And nobody at all is never.
            // With the countdown held rather than reset.
            // So a building unstaffed for a thousand ticks owes none.
            const auto period =
                workedPeriod(kSpawnPeriodTicks, staffingOf(world, entity));

            if (!period.has_value())
            {
                continue;
            }

            if (building.ticksUntilSpawn > 0)
            {
                // Copied and adjusted rather than rebuilt.
                // So a member added later is carried across.
                auto waiting = building;
                waiting.ticksUntilSpawn = building.ticksUntilSpawn - 1;
                world.set<Building>(entity, waiting);
                continue;
            }

            // Held at zero, not reset.
            // A house with no road is ready and waiting rather than owed.
            const auto onto = spawnCellFor(
                world.get<Cell>(entity),
                footprintOf(building.kind),
                paths);
            const auto slot = freeWalkerSlot(world, building);

            if (!onto.has_value() || !slot.has_value()
                || out >= kWalkerLimit)
            {
                continue;
            }

            const auto carries = carriedResource(*sends).has_value();

            const auto walker = world.create();
            world.add<Cell>(walker, *onto);
            world.add<Walker>(
                walker,
                Walker{
                    .kind = *sends,
                    .carried = carries ? kWalkerLoad : 0,
                    .home = entity});
            ++out;

            auto sent = building;
            sent.ticksUntilSpawn = *period - 1;
            sent.walkers[*slot] = walker;
            world.set<Building>(entity, sent);
        }
    }

} // namespace antwika::game
