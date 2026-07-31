#include "antwika/game/BuildingSystem.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <map>

#include <antwika/ecs/Entity.hpp>

#include "antwika/game/Building.hpp"
#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Direction.hpp"
#include "antwika/game/Resource.hpp"
#include "antwika/game/Walker.hpp"

namespace antwika::game
{

    namespace
    {
        using antwika::ecs::Entity;

        using Standing = std::map<Cell, Entity>;
        using Pending = std::map<Entity, Building>;

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

        // What ends a building: bad luck, or an empty larder.
        // A source holds stock nobody drains, so only risk takes one.
        [[nodiscard]] bool isLost(const Building &building)
        {
            if (building.risk >= kMaxRisk)
            {
                return true;
            }

            if (!consumes(building.kind))
            {
                return false;
            }

            return std::ranges::any_of(
                building.stock,
                [](std::int32_t held) { return held <= 0; });
        }

        void deliverTo(
            Building &building, Walker &walker, Resource resource)
        {
            auto &held = building.stock[resourceIndex(resource)];
            const auto room = kStockCapacity - held;
            const auto given = std::min(walker.carried, room);

            if (given <= 0)
            {
                return;
            }

            held += given;
            walker.carried -= given;
        }

        void deliver(World &world, const Standing &standing, Pending &pending)
        {
            for (const auto entity : world.view<Walker, Cell>())
            {
                auto walker = world.get<Walker>(entity);
                const auto carries = carriedResource(walker.kind);
                const auto at = world.get<Cell>(entity);
                const auto before = walker.carried;

                for (std::size_t index = 0; index < kDirectionCount; ++index)
                {
                    const auto beside =
                        step(at, static_cast<Direction>(index));
                    const auto found = standing.find(beside);

                    if (found == standing.end())
                    {
                        continue;
                    }

                    auto &building = touch(world, pending, found->second);

                    if (!carries.has_value())
                    {
                        // A fireman and an architect carry nothing.
                        // What they do instead is take risk off.
                        building.risk =
                            std::max(0, building.risk - kRiskRelief);
                        continue;
                    }

                    deliverTo(building, walker, *carries);
                }

                if (walker.carried != before)
                {
                    world.set<Walker>(entity, walker);
                }
            }
        }

        void age(World &world, Pending &pending)
        {
            for (const auto entity : world.view<Building, Cell>())
            {
                auto &building = touch(world, pending, entity);

                if (building.ticksUntilDrain > 0)
                {
                    --building.ticksUntilDrain;
                }
                else
                {
                    building.ticksUntilDrain = kDrainPeriodTicks;

                    // Only a house eats; a source keeps what it holds.
                    if (consumes(building.kind))
                    {
                        for (auto &held : building.stock)
                        {
                            held = std::max(0, held - 1);
                        }
                    }
                }

                if (building.ticksUntilRisk > 0)
                {
                    --building.ticksUntilRisk;
                    continue;
                }

                building.ticksUntilRisk = kRiskPeriodTicks;
                building.risk = std::min(kMaxRisk, building.risk + 1);
            }
        }
    } // namespace

    BuildingSystem::BuildingSystem(BuildingIndex &built) : built(built)
    {
    }

    void BuildingSystem::update(World &world, antwika::time::Tick)
    {
        // Where each building is, so a neighbour is a lookup.
        // Rather than a scan of every building for every walker.
        Standing standing;

        for (const auto entity : world.view<Building, Cell>())
        {
            standing.emplace(world.get<Cell>(entity), entity);
        }

        Pending pending;

        deliver(world, standing, pending);
        age(world, pending);

        for (const auto &[entity, building] : pending)
        {
            if (isLost(building))
            {
                built.erase(world.get<Cell>(entity));
                world.destroy(entity);
                continue;
            }

            world.set<Building>(entity, building);
        }
    }

} // namespace antwika::game
