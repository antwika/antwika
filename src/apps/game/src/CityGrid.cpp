#include "antwika/game/CityGrid.hpp"

#include <cstddef>
#include <map>
#include <optional>
#include <vector>

#include <antwika/ecs/Entity.hpp>

#include "antwika/game/Footprint.hpp"
#include "antwika/game/Path.hpp"

namespace antwika::game
{

    using antwika::ecs::Entity;
    using antwika::ecs::kNullEntity;

    CityGrid cityGridOf(const World &world)
    {
        CityGrid grid;

        // Walked once each, keeping where every entity landed.
        // So the second pass turns a handle into a record's index.
        std::map<Entity, std::size_t> walkerAt;
        std::map<Entity, std::size_t> buildingAt;

        for (const auto entity : world.view<Walker, Cell>())
        {
            walkerAt.emplace(entity, grid.walkers.size());

            // Cleared rather than carried across.
            // A handle put away here would name a destroyed entity.
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

            // The unwind pad of a record with a vector member.
            // See docs/confirming-unreachable-branches.md, (a).
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

        // The links, kept only where both ends were put away.
        // A building whose walker is gone has that slot free.
        // Which is exactly what it will be on the way back.
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

        // The two labour ledgers, resolved exactly as the homes are.
        // An entry naming a house that was not put away is dropped.
        // Which is an entry naming nothing a reopened city can hold.
        for (const auto entity : world.view<Building, Cell>())
        {
            if (world.has<Staff>(entity))
            {
                const auto &staff = world.get<Staff>(entity);
                // The unwind pad of a record with a vector member.
                // See docs/confirming-unreachable-branches.md, (a).
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
                // The unwind pad of a record with a vector member.
                // See docs/confirming-unreachable-branches.md, (a).
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

        // And the errands', on exactly those terms.
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

        // And the journeys', on exactly those terms again.
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

        return grid;
        // The excluded line is the local grid's unwind destructor.
        // Nothing between its construction and the return throws.
    } // GCOVR_EXCL_LINE

    void restoreCityGrid(
        World &world,
        BuildingIndex &built,
        const PathIndex &paths,
        const CityGrid &grid)
    {
        // Collected before anything is staged.
        // Destroying mid-walk would be walking what is being changed.
        std::vector<Entity> standing;
        for (const auto entity : world.view<Cell>())
        {
            standing.push_back(entity);
        }

        for (const auto entity : standing)
        {
            world.destroy(entity);
        }

        // Rebuilt rather than swapped in beside the world.
        // So it describes the live city by construction.
        built = BuildingIndex{};

        for (const auto cell : paths.cells())
        {
            const auto entity = world.create();
            world.add<Cell>(entity, cell);
            world.add<Path>(entity, Path{});
        }

        // **Every entity is created before any component is added.**
        // create() is immediate where add() is staged.
        // So the handles exist now and the components do not.
        // A link therefore has to be built into the component.
        // Reading one back would ask for what has not been given yet.
        std::vector<Entity> walkers;
        std::vector<Entity> buildings;

        walkers.reserve(grid.walkers.size());
        buildings.reserve(grid.buildings.size());

        for (std::size_t index = 0; index < grid.walkers.size(); ++index)
        {
            walkers.push_back(world.create());
        }

        for (std::size_t index = 0; index < grid.buildings.size(); ++index)
        {
            buildings.push_back(world.create());
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

            // Only where there is something in it.
            // An absent component already means uncovered.
            // So a city nobody served comes back with none.
            if (stored.coverage != Coverage{})
            {
                setCoverage(world, buildings[index], stored.coverage);
            }

            if (stored.production.has_value())
            {
                world.add<Production>(
                    buildings[index], *stored.production);
            }

            // Through the one writer, exactly as the coverage is.
            // So World::add<Household> lives in one translation unit.
            if (stored.household.has_value())
            {
                setHousehold(
                    world, buildings[index], *stored.household);
            }

            // And the two labour ledgers, through their one writers.
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

} // namespace antwika::game
