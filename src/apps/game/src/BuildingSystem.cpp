#include "antwika/game/BuildingSystem.hpp"

#include <array>

#include "antwika/game/Cell.hpp"
#include "antwika/game/Direction.hpp"

namespace antwika::game
{

    namespace
    {
        // North, east, south, west, and always in that order.
        // A spawn picks one road out of up to four.
        // A less definite pick would not replay.
        constexpr std::array<Direction, kDirectionCount> kSearchOrder{
            Direction::North,
            Direction::East,
            Direction::South,
            Direction::West};

        // Nothing left, burnt down, or fallen over.
        // One rule, because all three end the same way.
        [[nodiscard]] bool gone(const Building &building) noexcept
        {
            return building.stock.held <= 0
                   || building.fireRisk >= kMaxRisk
                   || building.collapseRisk >= kMaxRisk;
        }
    } // namespace

    BuildingSystem::BuildingSystem(
        const PathIndex &paths, BuildingIndex &buildings)
        : paths(paths), buildings(buildings)
    {
    }

    void BuildingSystem::update(World &world, antwika::time::Tick)
    {
        for (const auto entity : world.view<Building, Cell>())
        {
            const auto at = world.get<Cell>(entity);
            auto value = world.get<Building>(entity);

            if (due(value.drainIn, kDrainPeriodTicks))
            {
                --value.stock.held;
            }

            if (due(value.riskIn, kRiskPeriodTicks))
            {
                // Not clamped at kMaxRisk.
                // Reaching it is the end of the building.
                // gone() takes it out on this very tick.
                // So no risk ever sits at the maximum to be capped.
                ++value.fireRisk;
                ++value.collapseRisk;
            }

            const auto spawns = walkerFor(value.kind);
            if (spawns.has_value()
                && due(value.spawnIn, kSpawnPeriodTicks))
            {
                // No road to step out onto yet.
                // So try again next tick rather than in another minute.
                if (!spawn(world, at, *spawns))
                {
                    value.spawnIn = 1;
                }
            }

            if (gone(value))
            {
                world.destroy(entity);
                buildings.erase(at);
                continue;
            }

            world.set<Building>(entity, value);
        }
    }

    bool BuildingSystem::spawn(World &world, Cell at, WalkerKind kind)
    {
        for (const auto direction : kSearchOrder)
        {
            const auto onto = step(at, direction);
            if (!paths.has(onto))
            {
                continue;
            }

            const auto entity = world.create();
            world.add<Cell>(entity, onto);
            // Its way home is the road it steps out onto, not `at`.
            // A building's own cell has no road to stand on.
            world.add<Walker>(
                entity, newlySpawned(kind, direction, onto));

            return true;
        }

        return false;
    }

} // namespace antwika::game
