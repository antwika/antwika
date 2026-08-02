#include "antwika/game/BuildingSystem.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <map>

#include <antwika/ecs/Entity.hpp>

#include "antwika/game/Building.hpp"
#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Coverage.hpp"
#include "antwika/game/Direction.hpp"
#include "antwika/game/Footprint.hpp"
#include "antwika/game/Resource.hpp"
#include "antwika/game/Service.hpp"
#include "antwika/game/StandingBuildings.hpp"
#include "antwika/game/Walker.hpp"

namespace antwika::game
{

    namespace
    {
        using antwika::ecs::Entity;

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
        // And only what sustains() names is a larder.
        // A house holding no clay is a house nobody has carted to yet.
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
                kResources,
                [&building](Resource resource)
                {
                    return sustains(resource)
                        && building.stock[resourceIndex(resource)] <= 0;
                });
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

        // No guard against serving one building twice.
        // A rectangle cannot be touched twice from one outside cell.
        // Two of a cell's neighbours in it would put the cell in it.
        // An opposite pair spans the cell in one axis.
        // A perpendicular pair spans it in both.
        // So it would be a road under a building, which nothing places.
        void deliver(
            World &world,
            const StandingBuildings &standing,
            Pending &pending)
        {
            for (const auto entity : world.view<Walker, Cell>())
            {
                auto walker = world.get<Walker>(entity);
                const auto carries = carriedResource(walker.kind);

                // A walker carrying nothing fixed hands nothing over.
                // It used to take risk off instead.
                // Which was coverage said as a subtraction.
                // It refreshes coverage now, in CoverageSystem.
                // So a delivery has nothing to do with one at all.
                if (!carries.has_value())
                {
                    continue;
                }

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

                    deliverTo(
                        touch(world, pending, found->second),
                        walker,
                        *carries);
                }

                if (walker.carried != before)
                {
                    world.set<Walker>(entity, walker);
                }
            }
        }

        // What holds a building's risk off, and the reason those two.
        // Safety is the fireman's and Structure is the engineer's.
        // Both used to relieve risk by walking past a building.
        // So the two services are exactly the old special cases.
        // Water and Health reach a house for what a house does with them.
        // Neither has anything to say about the building falling down.
        constexpr std::array<Service, 2> kRiskHeldOffBy{
            Service::Safety, Service::Structure};

        // Read as of the last commit.
        // Which is what the "serve" phase left at the previous tick.
        // One tick out of five hundred on a "came recently" countdown.
        [[nodiscard]] bool unserved(const World &world, Entity entity)
        {
            return std::ranges::any_of(
                kRiskHeldOffBy,
                [&world, entity](Service service)
                { return coverageOf(world, entity, service) <= 0; });
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

                // **The one thing that changed about risk.**
                // It used to rise here whatever was standing nearby.
                // And a passing fireman subtracted a lump from it.
                // Now it rises where the district is unserved.
                // And falls back where a walker keeps reaching it.
                // So a fire station is a thing a district has.
                // Rather than a thing that visits it.
                building.ticksUntilRisk = kRiskPeriodTicks;
                building.risk = unserved(world, entity)
                    ? std::min(kMaxRisk, building.risk + 1)
                    : std::max(0, building.risk - 1);
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
        const auto standing = standingBuildings(world);

        Pending pending;

        deliver(world, standing, pending);
        age(world, pending);

        for (const auto &[entity, building] : pending)
        {
            if (isLost(building))
            {
                built.erase(
                    world.get<Cell>(entity), footprintOf(building.kind));
                world.destroy(entity);
                continue;
            }

            world.set<Building>(entity, building);
        }
    }

} // namespace antwika::game
