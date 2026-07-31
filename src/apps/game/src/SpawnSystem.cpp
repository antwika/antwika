#include "antwika/game/SpawnSystem.hpp"

#include <array>
#include <cstdint>

#include "antwika/game/Building.hpp"
#include "antwika/game/Direction.hpp"
#include "antwika/game/Walker.hpp"

namespace antwika::game
{

    SpawnSystem::SpawnSystem(const PathIndex &paths) : paths(paths)
    {
    }

    std::optional<Cell> spawnCellFor(Cell at, const PathIndex &paths)
    {
        std::optional<Cell> best;

        for (std::size_t index = 0; index < kDirectionCount; ++index)
        {
            const Cell beside =
                step(at, static_cast<Direction>(index));

            if (!paths.has(beside))
            {
                continue;
            }

            // The lowest neighbour, in Cell's own ordering.
            // Two roads beside one house must pick the same one always.
            if (!best.has_value() || beside < *best)
            {
                best = beside;
            }
        }

        return best;
    }

    void SpawnSystem::update(World &world, antwika::time::Tick)
    {
        // Counted once rather than per building.
        // Every walker staged this tick is one the cap has to see.
        std::size_t out = world.view<Walker>().size();

        for (const auto entity : world.view<Building, Cell>())
        {
            const auto building = world.get<Building>(entity);

            if (!spawnsWalkers(building.kind))
            {
                continue;
            }

            if (building.ticksUntilSpawn > 0)
            {
                world.set<Building>(
                    entity,
                    Building{
                        .kind = building.kind,
                        .ticksUntilSpawn = static_cast<std::uint8_t>(
                            building.ticksUntilSpawn - 1)});
                continue;
            }

            // Held at zero, not reset.
            // A house with no road is ready and waiting rather than owed.
            const auto onto =
                spawnCellFor(world.get<Cell>(entity), paths);

            if (!onto.has_value() || out >= kWalkerLimit)
            {
                continue;
            }

            const auto walker = world.create();
            world.add<Cell>(walker, *onto);
            world.add<Walker>(walker, Walker{});
            ++out;

            world.set<Building>(
                entity,
                Building{
                    .kind = building.kind,
                    .ticksUntilSpawn = static_cast<std::uint8_t>(
                        kTicksPerSpawn - 1)});
        }
    }

} // namespace antwika::game
