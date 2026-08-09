#include "antwika/game/PopulationSystem.hpp"

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <utility>

#include <antwika/ecs/Entity.hpp>

#include "antwika/game/Building.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Coverage.hpp"
#include "antwika/game/Footprint.hpp"
#include "antwika/game/Household.hpp"
#include "antwika/game/HousingLevel.hpp"
#include "antwika/game/HousingQuery.hpp"
#include "antwika/game/Journey.hpp"
#include "antwika/game/Resource.hpp"
#include "antwika/game/Service.hpp"
#include "antwika/game/SpawnSystem.hpp"
#include "antwika/game/Walker.hpp"

namespace antwika::game
{

    namespace
    {
        using antwika::ecs::Entity;
        using antwika::ecs::kNullEntity;

        [[nodiscard]] bool starved(
            const Building &building, const GameConfig &config)
        {
            for (const auto resource : kResources)
            {
                if (config.sustaining[resourceIndex(resource)]
                    && building.stock[resourceIndex(resource)] <= 0)
                {
                    return true;
                }
            }

            return false;
        }

        [[nodiscard]] std::int32_t comingTo(
            const std::map<Entity, std::int32_t> &expected, Entity house)
        {
            const auto found = expected.find(house);

            return found != expected.end() ? found->second : 0;
        }

        [[nodiscard]] bool migrantDue(
            antwika::time::Tick tick, std::int32_t period) noexcept
        {
            return tick % static_cast<antwika::time::Tick>(period) == 0;
        }
    }

    PopulationSystem::PopulationSystem(
        const PathIndex &paths,
        const BuildingIndex &built,
        const DesirabilityField &desirability,
        GridExtent extent,
        GameConfig config) noexcept
        : paths(paths),
          built(built),
          desirability(desirability),
          extent(extent),
          config(config)
    {
    }

    void PopulationSystem::update(World &world, antwika::time::Tick tick)
    {
        const auto arrivals = admit(world);

        const bool recruiting =
            migrantDue(tick, config.migrantPeriodTicks);

        const auto expected = expecting(world);

        for (const auto entity : world.view<Building, Cell>())
        {
            const auto building = world.get<Building>(entity);

            if (!housesPeople(building.kind))
            {
                continue;
            }

            auto household = householdOf(world, entity);

            const auto arriving = arrivals.find(entity);

            if (arriving != arrivals.end())
            {
                const auto room = populationCapacityOf(household.level)
                    - household.population;
                household.population +=
                    std::min(arriving->second, std::max(room, 0));
            }

            if (recruiting)
            {
                settle(world, entity, building, household);

                recruit(
                    world,
                    entity,
                    building,
                    household,
                    comingTo(expected, entity));
            }

            setHousehold(world, entity, household);
        }
    }

    PopulationSystem::Standing PopulationSystem::standingOf(
        World &world,
        Entity entity,
        const Building &building,
        const Household &household)
    {
        const auto at = world.get<Cell>(entity);

        const bool dry =
            coverageOf(world, entity, Service::Water) <= 0;

        return Standing{
            .capacity = populationCapacityOf(household.level),
            .pleasant = desirabilityAt(desirability, at)
                >= requirementOf(household.level).desirability,
            .unlivable = starved(building, config) || dry,
            .door = spawnCellFor(at, footprintOf(building.kind), paths)};
    }

    void PopulationSystem::settle(
        World &world,
        Entity entity,
        const Building &building,
        Household &household)
    {
        const auto standing = standingOf(world, entity, building, household);

        if (standing.pleasant && !standing.unlivable
            && household.population <= standing.capacity)
        {
            return;
        }

        if (household.population <= 0)
        {
            return;
        }

        --household.population;
        turnOut(world, entity, standing.door, standing.unlivable);
    }

    void PopulationSystem::recruit(
        World &world,
        Entity entity,
        const Building &building,
        const Household &household,
        std::int32_t coming)
    {
        const auto standing = standingOf(world, entity, building, household);

        if (!standing.pleasant || standing.unlivable
            || !standing.door.has_value() || coming > 0
            || household.population >= standing.capacity)
        {
            return;
        }

        send(
            world,
            entity,
            *standing.door,
            standing.capacity - household.population);
    }

    void PopulationSystem::send(
        World &world,
        Entity entity,
        Cell door,
        const std::int32_t carried)
    {
        const auto gate = nearestGate(door, built, extent);
        const auto out = world.view<Walker>().size();

        if (!gate.has_value() || out >= config.walkerLimit)
        {
            return;
        }

        const auto migrant = world.create();
        world.add<Cell>(migrant, *gate);
        world.add<Walker>(
            migrant,
            Walker{.kind = WalkerKind::Migrant, .carried = carried});
        world.add<Journey>(
            migrant,
            Journey{
                .towards = world.get<Cell>(entity), .house = entity});
    }

    void PopulationSystem::turnOut(
        World &world,
        Entity entity,
        std::optional<Cell> door,
        bool emigrates)
    {
        const auto out = world.view<Walker>().size();

        if (!door.has_value() || out >= config.walkerLimit)
        {
            return;
        }

        const auto vacancy = emigrates
            ? kNullEntity
            : nearestVacancy(world, *door, entity, built, extent);

        const auto towards = vacancy != kNullEntity
            ? std::optional<Cell>(world.get<Cell>(vacancy))
            : nearestGate(*door, built, extent);

        if (!towards.has_value())
        {
            return;
        }

        const auto leaver = world.create();
        world.add<Cell>(leaver, *door);

        world.add<Walker>(
            leaver,
            Walker{.kind = WalkerKind::Migrant, .carried = 1});
        world.add<Journey>(
            leaver, Journey{.towards = *towards, .house = vacancy});
    }

    std::map<Entity, std::int32_t> PopulationSystem::admit(World &world)
    {
        std::map<std::pair<Cell, Entity>, Entity> arrived;

        for (const auto entity : world.view<Walker, Cell, Journey>())
        {
            const auto journey = world.get<Journey>(entity);

            if (journey.house == kNullEntity
                || !world.has<Cell>(journey.house))
            {
                continue;
            }

            if (!beside(
                    world.get<Cell>(entity),
                    world.get<Cell>(journey.house),
                    footprintOf(
                        world.get<Building>(journey.house).kind)))
            {
                continue;
            }

            arrived.emplace(
                std::pair{world.get<Cell>(entity), entity},
                journey.house);
        }

        std::map<Entity, std::int32_t> counted;

        for (const auto &[key, house] : arrived)
        {
            counted[house] += world.get<Walker>(key.second).carried;
            world.destroy(key.second);
        }

        return counted;
    } // GCOVR_EXCL_LINE

    std::map<Entity, std::int32_t> PopulationSystem::expecting(
        const World &world)
    {
        std::map<Entity, std::int32_t> counted;

        for (const auto entity : world.view<Walker, Journey>())
        {
            const auto house = world.get<Journey>(entity).house;

            if (house != kNullEntity)
            {
                ++counted[house];
            }
        }

        return counted;
    } // GCOVR_EXCL_LINE

}
