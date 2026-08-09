#include "antwika/game/BuildingSystem.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <map>
#include <set>
#include <utility>

#include <antwika/ecs/Entity.hpp>

#include "antwika/game/Building.hpp"
#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Coverage.hpp"
#include "antwika/game/Demolition.hpp"
#include "antwika/game/Direction.hpp"
#include "antwika/game/Errand.hpp"
#include "antwika/game/Footprint.hpp"
#include "antwika/game/HousingQuery.hpp"
#include "antwika/game/Resource.hpp"
#include "antwika/game/Service.hpp"
#include "antwika/game/StandingBuildings.hpp"
#include "antwika/game/Store.hpp"
#include "antwika/game/Walker.hpp"

namespace antwika::game
{

    namespace
    {
        using antwika::ecs::Entity;
        using antwika::ecs::kNullEntity;

        using Pending = std::map<Entity, Building>;

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

        [[nodiscard]] bool catchesFire(const Building &building)
        {
            return building.fireRisk >= kMaxRisk;
        }

        [[nodiscard]] bool collapses(const Building &building)
        {
            return building.collapseRisk >= kMaxRisk;
        }

        void deliverTo(
            Building &building,
            Walker &walker,
            Resource resource,
            std::int32_t capacity)
        {
            auto &held = building.stock[resourceIndex(resource)];
            const auto room = capacity - held;
            const auto given = std::min(walker.carried, room);

            if (given <= 0)
            {
                return;
            }

            held += given;
            walker.carried -= given;
        }

        [[nodiscard]] StandingBuildings neighboursOf(
            const StandingBuildings &standing, Cell at)
        {
            StandingBuildings beside;

            for (std::size_t index = 0; index < kDirectionCount; ++index)
            {
                const auto cell = step(at, static_cast<Direction>(index));
                const auto found = standing.find(cell);

                if (found != standing.end())
                {
                    beside.emplace(cell, found->second);
                }
            }

            return beside;
        } // GCOVR_EXCL_LINE

        void deliver(
            World &world,
            const StandingBuildings &standing,
            Pending &pending)
        {
            std::set<std::pair<Cell, Entity>> movers;

            for (const auto entity : world.view<Walker, Cell>())
            {
                movers.emplace(world.get<Cell>(entity), entity);
            }

            for (const auto &[at, entity] : movers)
            {
                if (!world.has<Errand>(entity))
                {
                    continue;
                }

                const auto errand = world.get<Errand>(entity);

                if (errand.leg == ErrandLeg::Returning)
                {
                    continue;
                }

                auto walker = world.get<Walker>(entity);
                const auto carries = errand.carrying;
                const auto bound = errand.destination;
                const auto before = walker.carried;

                for (const auto &[cell, standingHere] :
                     neighboursOf(standing, at))
                {
                    if (bound != kNullEntity && standingHere != bound)
                    {
                        continue;
                    }

                    auto &building = touch(world, pending, standingHere);

                    if (bound == kNullEntity && !consumes(building.kind))
                    {
                        continue;
                    }

                    deliverTo(
                        building,
                        walker,
                        carries,
                        stockCapacityAt(
                            world, standingHere, building.kind));
                }

                if (walker.carried != before)
                {
                    world.set<Walker>(entity, walker);
                }
            }
        }

        void relieve(
            World &world,
            const StandingBuildings &standing,
            Pending &pending)
        {
            for (const auto entity : world.view<Walker, Cell>())
            {
                const auto kind = world.get<Walker>(entity).kind;

                if (kind != WalkerKind::Fireman
                    && kind != WalkerKind::Engineer)
                {
                    continue;
                }

                const auto at = world.get<Cell>(entity);

                for (const auto &[cell, standingHere] :
                     neighboursOf(standing, at))
                {
                    auto &building = touch(world, pending, standingHere);

                    if (kind == WalkerKind::Fireman)
                    {
                        building.fireRisk = 0;
                    }
                    else
                    {
                        building.collapseRisk = 0;
                    }
                }
            }
        }

        [[nodiscard]] std::int32_t steppedDisease(
            const World &world, Entity entity, std::int32_t risk)
        {
            return coverageOf(world, entity, Service::Health) <= 0
                ? std::min(kMaxRisk, risk + 1)
                : std::max(0, risk - 1);
        }

        void age(
            World &world, Pending &pending, const GameConfig &config)
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
                    building.ticksUntilDrain = config.drainPeriodTicks;

                    if (housesPeople(building.kind))
                    {
                        const auto eaten =
                            (populationAt(world, entity)
                             + config.mouthsPerServing - 1)
                            / config.mouthsPerServing;

                        for (auto &held : building.stock)
                        {
                            held = std::max(0, held - eaten);
                        }
                    }
                }

                if (building.ticksUntilRisk > 0)
                {
                    --building.ticksUntilRisk;
                    continue;
                }

                building.ticksUntilRisk = config.riskPeriodTicks;
                building.fireRisk =
                    std::min(kMaxRisk, building.fireRisk + 1);
                building.collapseRisk =
                    std::min(kMaxRisk, building.collapseRisk + 1);
                building.diseaseRisk = steppedDisease(
                    world, entity, building.diseaseRisk);
            }
        }
    }

    BuildingSystem::BuildingSystem(
        BuildingIndex &built, GridExtent extent, GameConfig config)
        : built(built), extent(extent), config(config)
    {
    }

    void BuildingSystem::update(World &world, antwika::time::Tick)
    {
        const auto standing = standingBuildings(world);

        Pending pending;

        deliver(world, standing, pending);
        age(world, pending, config);

        relieve(world, standing, pending);

        enum class Ending : std::uint8_t
        {
            Burns,
            Falls,
        };

        std::map<Cell, std::pair<Entity, Ending>> lost;

        for (const auto &[entity, building] : pending)
        {
            if (catchesFire(building))
            {
                lost.emplace(
                    world.get<Cell>(entity),
                    std::make_pair(entity, Ending::Burns));
                continue;
            }

            if (collapses(building))
            {
                lost.emplace(
                    world.get<Cell>(entity),
                    std::make_pair(entity, Ending::Falls));
                continue;
            }

            world.set<Building>(entity, building);
        }

        for (const auto &[at, ending] : lost)
        {
            if (ending.second == Ending::Burns)
            {
                ignite(world, built, ending.first, extent, config);
            }
            else
            {
                collapse(world, built, ending.first, extent, config);
            }
        }
    }

}
