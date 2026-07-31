#include "antwika/game/SessionStore.hpp"

#include "antwika/game/Building.hpp"
#include "antwika/game/CityGrid.hpp"
#include "antwika/game/Walker.hpp"

namespace antwika::game
{

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
        paths = pathIndexOf(save);
        camera = save.camera;
        state = save.state;

        // Translated rather than laid down here.
        // Putting entities on the live grid is one piece of code.
        // The city switch uses the very same one -- see CityGrid.
        // So the create-before-add rule is stated once.
        CityGrid grid;
        grid.walkers.reserve(save.walkers.size());
        grid.buildings.reserve(save.buildings.size());

        for (const auto &walker : save.walkers)
        {
            grid.walkers.push_back(
                StoredWalker{
                    .at = walker.at,
                    .walker =
                        Walker{
                            .facing = walker.facing,
                            .kind = walker.kind,
                            .carried = walker.carried,
                            .stepsUntilHome = walker.stepsUntilHome,
                            .ticksUntilStep = walker.ticksUntilStep},
                    .home = walker.home});
        }

        for (const auto &building : save.buildings)
        {
            grid.buildings.push_back(
                StoredBuilding{
                    .at = building.at,
                    .building =
                        Building{
                            .kind = building.kind,
                            .stock = building.stock,
                            .risk = building.risk,
                            .ticksUntilSpawn = building.ticksUntilSpawn,
                            .ticksUntilDrain = building.ticksUntilDrain,
                            .ticksUntilRisk = building.ticksUntilRisk},
                    .walker = building.walker});
        }

        restoreCityGrid(world, built, paths, grid);
    }

} // namespace antwika::game
