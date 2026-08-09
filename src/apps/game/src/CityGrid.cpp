#include "antwika/game/CityGrid.hpp"

#include <cstddef>
#include <map>
#include <optional>
#include <vector>

#include <antwika/ecs/Entity.hpp>

#include "antwika/game/FireCall.hpp"
#include "antwika/game/Footprint.hpp"
#include "antwika/game/Path.hpp"

namespace antwika::game
{

    using antwika::ecs::Entity;
    using antwika::ecs::kNullEntity;

    CityGrid cityGridOf(const World &world)
    {
        CityGrid grid;

        std::map<Entity, std::size_t> walkerAt;
        std::map<Entity, std::size_t> buildingAt;
        std::map<Entity, std::size_t> ruinAt;

        for (const auto entity : world.view<Walker, Cell>())
        {
            walkerAt.emplace(entity, grid.walkers.size());

            auto walker = world.get<Walker>(entity);
            walker.home = kNullEntity;

            std::optional<Errand> errand;

            if (world.has<Errand>(entity))
            {
                errand = world.get<Errand>(entity);
                errand->destination = kNullEntity;
            }

            std::optional<Journey> journey;

            if (world.has<Journey>(entity))
            {
                journey = world.get<Journey>(entity);
                journey->house = kNullEntity;
            }

            grid.walkers.push_back(
                StoredWalker{
                    .at = world.get<Cell>(entity),
                    .walker = walker,
                    .errand = errand,
                    .journey = journey});
        }

        for (const auto entity : world.view<Building, Cell>())
        {
            buildingAt.emplace(entity, grid.buildings.size());

            auto building = world.get<Building>(entity);
            building.walkers = {};

            std::optional<Production> production;

            if (world.has<Production>(entity))
            {
                production = world.get<Production>(entity);
            }

            std::optional<Household> household;

            if (world.has<Household>(entity))
            {
                household = world.get<Household>(entity);
            }

            // GCOVR_EXCL_START
            grid.buildings.push_back(
                StoredBuilding{
                    .at = world.get<Cell>(entity),
                    .building = building,
                    .walkers = {},
                    .coverage = coverageOf(world, entity),
                    .production = production,
                    .household = household});
            // GCOVR_EXCL_STOP
        }

        for (const auto entity : world.view<Ruin, Cell>())
        {
            ruinAt.emplace(entity, grid.ruins.size());

            grid.ruins.push_back(
                StoredRuin{
                    .at = world.get<Cell>(entity),
                    .ruin = world.get<Ruin>(entity)});
        }

        for (const auto entity : world.view<Building, Cell>())
        {
            const auto held = world.get<Building>(entity).walkers;

            for (std::size_t slot = 0; slot < kMaxWalkersOut; ++slot)
            {
                const auto found = walkerAt.find(held[slot]);

                if (found == walkerAt.end())
                {
                    continue;
                }

                grid.buildings[buildingAt.at(entity)].walkers[slot] =
                    found->second;
                grid.walkers[found->second].home = buildingAt.at(entity);
            }
        }

        for (const auto entity : world.view<Building, Cell>())
        {
            if (world.has<Staff>(entity))
            {
                const auto &staff = world.get<Staff>(entity);
                // GCOVR_EXCL_START
                StoredStaff stored{
                    .entries = {},
                    .ticksUntilDecay = staff.ticksUntilDecay};
                // GCOVR_EXCL_STOP

                for (const auto &entry : staff.sources)
                {
                    const auto found = buildingAt.find(entry.house);

                    if (entry.count > 0 && found != buildingAt.end())
                    {
                        stored.entries.push_back(
                            StoredStaffEntry{
                                .house = found->second,
                                .count = entry.count});
                    }
                }

                grid.buildings[buildingAt.at(entity)].staff = stored;
            }

            if (world.has<Employment>(entity))
            {
                const auto &employment = world.get<Employment>(entity);
                // GCOVR_EXCL_START
                StoredEmployment stored{
                    .jobs = {},
                    .ticksUntilDispatch =
                        employment.ticksUntilDispatch};
                // GCOVR_EXCL_STOP

                for (const auto &holding : employment.jobs)
                {
                    const auto found =
                        buildingAt.find(holding.workplace);

                    if (holding.count > 0 && found != buildingAt.end())
                    {
                        stored.jobs.push_back(
                            StoredJob{
                                .workplace = found->second,
                                .count = holding.count});
                    }
                }

                grid.buildings[buildingAt.at(entity)].employment =
                    stored;
            }
        }

        for (const auto entity : world.view<Walker, Cell>())
        {
            if (!world.has<Errand>(entity))
            {
                continue;
            }

            const auto found =
                buildingAt.find(world.get<Errand>(entity).destination);

            if (found == buildingAt.end())
            {
                continue;
            }

            grid.walkers[walkerAt.at(entity)].destination = found->second;
        }

        for (const auto entity : world.view<Walker, Cell>())
        {
            if (!world.has<Journey>(entity))
            {
                continue;
            }

            const auto found =
                buildingAt.find(world.get<Journey>(entity).house);

            if (found == buildingAt.end())
            {
                continue;
            }

            grid.walkers[walkerAt.at(entity)].joining = found->second;
        }

        for (const auto entity : world.view<Walker, Cell>())
        {
            if (!world.has<FireCall>(entity))
            {
                continue;
            }

            const auto found =
                ruinAt.find(world.get<FireCall>(entity).target);

            if (found == ruinAt.end())
            {
                continue;
            }

            grid.walkers[walkerAt.at(entity)].attending = found->second;
        }

        return grid;
    } // GCOVR_EXCL_LINE

    void restoreCityGrid(
        World &world,
        BuildingIndex &built,
        const PathIndex &paths,
        const CityGrid &grid)
    {
        std::vector<Entity> standing;
        for (const auto entity : world.view<Cell>())
        {
            standing.push_back(entity);
        }

        for (const auto entity : standing)
        {
            world.destroy(entity);
        }

        built = BuildingIndex{};

        for (const auto cell : paths.cells())
        {
            const auto entity = world.create();
            world.add<Cell>(entity, cell);
            world.add<Path>(entity, Path{});
        }

        std::vector<Entity> walkers;
        std::vector<Entity> buildings;
        std::vector<Entity> ruins;

        walkers.reserve(grid.walkers.size());
        buildings.reserve(grid.buildings.size());
        ruins.reserve(grid.ruins.size());

        for (std::size_t index = 0; index < grid.walkers.size(); ++index)
        {
            walkers.push_back(world.create());
        }

        for (std::size_t index = 0; index < grid.buildings.size(); ++index)
        {
            buildings.push_back(world.create());
        }

        for (std::size_t index = 0; index < grid.ruins.size(); ++index)
        {
            ruins.push_back(world.create());
        }

        for (std::size_t index = 0; index < grid.walkers.size(); ++index)
        {
            const auto &stored = grid.walkers[index];

            auto walker = stored.walker;
            walker.home = stored.home.has_value()
                ? buildings[*stored.home]
                : kNullEntity;

            world.add<Cell>(walkers[index], stored.at);
            world.add<Walker>(walkers[index], walker);

            if (stored.journey.has_value())
            {
                auto journey = *stored.journey;
                journey.house = stored.joining.has_value()
                    ? buildings[*stored.joining]
                    : kNullEntity;

                world.add<Journey>(walkers[index], journey);
            }

            if (stored.attending.has_value())
            {
                world.add<FireCall>(
                    walkers[index],
                    FireCall{.target = ruins[*stored.attending]});
            }

            if (!stored.errand.has_value())
            {
                continue;
            }

            auto errand = *stored.errand;
            errand.destination = stored.destination.has_value()
                ? buildings[*stored.destination]
                : kNullEntity;

            world.add<Errand>(walkers[index], errand);
        }

        for (std::size_t index = 0; index < grid.ruins.size(); ++index)
        {
            const auto &stored = grid.ruins[index];

            world.add<Cell>(ruins[index], stored.at);
            world.add<Ruin>(ruins[index], stored.ruin);

            (void)built.insert(
                stored.at, footprintOf(stored.ruin.kind));
        }

        for (std::size_t index = 0; index < grid.buildings.size(); ++index)
        {
            const auto &stored = grid.buildings[index];

            auto building = stored.building;

            for (std::size_t slot = 0; slot < kMaxWalkersOut; ++slot)
            {
                building.walkers[slot] = stored.walkers[slot].has_value()
                    ? walkers[*stored.walkers[slot]]
                    : kNullEntity;
            }

            world.add<Cell>(buildings[index], stored.at);
            world.add<Building>(buildings[index], building);

            if (stored.coverage != Coverage{})
            {
                setCoverage(world, buildings[index], stored.coverage);
            }

            if (stored.production.has_value())
            {
                world.add<Production>(
                    buildings[index], *stored.production);
            }

            if (stored.household.has_value())
            {
                setHousehold(
                    world, buildings[index], *stored.household);
            }

            if (stored.staff.has_value())
            {
                Staff staff;
                staff.ticksUntilDecay = stored.staff->ticksUntilDecay;

                for (std::size_t slot = 0;
                     slot < stored.staff->entries.size()
                     && slot < kMaxStaffSources;
                     ++slot)
                {
                    staff.sources[slot] = StaffEntry{
                        .house =
                            buildings[stored.staff->entries[slot].house],
                        .count = stored.staff->entries[slot].count};
                }

                setStaff(world, buildings[index], staff);
            }

            if (stored.employment.has_value())
            {
                Employment employment;
                employment.ticksUntilDispatch =
                    stored.employment->ticksUntilDispatch;

                for (std::size_t slot = 0;
                     slot < stored.employment->jobs.size()
                     && slot < kMaxJobs;
                     ++slot)
                {
                    employment.jobs[slot] = JobHolding{
                        .workplace = buildings
                            [stored.employment->jobs[slot].workplace],
                        .count =
                            stored.employment->jobs[slot].count};
                }

                setEmployment(world, buildings[index], employment);
            }

            (void)built.insert(stored.at, footprintOf(building.kind));
        }
    }

}
