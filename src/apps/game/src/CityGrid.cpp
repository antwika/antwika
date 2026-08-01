#include "antwika/game/CityGrid.hpp"

#include <cstddef>
#include <map>
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

            grid.walkers.push_back(
                StoredWalker{
                    .at = world.get<Cell>(entity), .walker = walker});
        }

        for (const auto entity : world.view<Building, Cell>())
        {
            buildingAt.emplace(entity, grid.buildings.size());

            auto building = world.get<Building>(entity);
            building.walkers = {};

            grid.buildings.push_back(
                StoredBuilding{
                    .at = world.get<Cell>(entity), .building = building});
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

            (void)built.insert(stored.at, footprintOf(building.kind));
        }
    }

} // namespace antwika::game
