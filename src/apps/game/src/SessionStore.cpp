#include "antwika/game/SessionStore.hpp"

#include <cstddef>
#include <vector>

#include <antwika/ecs/Entity.hpp>

#include "antwika/game/Building.hpp"
#include "antwika/game/BuildingIndex.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Path.hpp"
#include "antwika/game/Walker.hpp"

namespace antwika::game
{

    using antwika::ecs::Entity;

    SessionStore::SessionStore(
        World &world,
        PathIndex &paths,
        BuildingIndex &built,
        Camera &camera,
        GameState &state,
        GridExtent extent,
        std::uint64_t seed)
        : world(world),
          paths(paths),
          built(built),
          camera(camera),
          state(state),
          extent(extent),
          seed(seed)
    {
    }

    SaveGame SessionStore::take() const
    {
        return saveGameOf(world, paths, camera, state, extent, seed);
    }

    void SessionStore::restore(const SaveGame &save)
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

        paths = pathIndexOf(save);
        built = BuildingIndex{};
        camera = save.camera;
        state = save.state;

        for (const auto cell : save.paths)
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

        walkers.reserve(save.walkers.size());
        buildings.reserve(save.buildings.size());

        for (std::size_t index = 0; index < save.walkers.size(); ++index)
        {
            walkers.push_back(world.create());
        }

        for (std::size_t index = 0; index < save.buildings.size(); ++index)
        {
            buildings.push_back(world.create());
        }

        for (std::size_t index = 0; index < save.walkers.size(); ++index)
        {
            const auto &walker = save.walkers[index];

            world.add<Cell>(walkers[index], walker.at);
            world.add<Walker>(
                walkers[index],
                Walker{
                    .facing = walker.facing,
                    .kind = walker.kind,
                    .carried = walker.carried,
                    .stepsUntilHome = walker.stepsUntilHome,
                    .home = walker.home.has_value()
                        ? buildings[*walker.home]
                        : antwika::ecs::kNullEntity,
                    .ticksUntilStep = walker.ticksUntilStep});
        }

        for (std::size_t index = 0; index < save.buildings.size(); ++index)
        {
            const auto &building = save.buildings[index];

            world.add<Cell>(buildings[index], building.at);
            world.add<Building>(
                buildings[index],
                Building{
                    .kind = building.kind,
                    .stock = building.stock,
                    .risk = building.risk,
                    .ticksUntilSpawn = building.ticksUntilSpawn,
                    .ticksUntilDrain = building.ticksUntilDrain,
                    .ticksUntilRisk = building.ticksUntilRisk,
                    .walker = building.walker.has_value()
                        ? walkers[*building.walker]
                        : antwika::ecs::kNullEntity});
            built.insert(building.at);
        }
    }

} // namespace antwika::game
