#include "antwika/game/SceneSnapshot.hpp"

#include "antwika/game/BuildGhost.hpp"
#include "antwika/game/Building.hpp"
#include "antwika/game/Walker.hpp"

namespace antwika::game
{

    SceneSnapshot snapshotOf(
        const World &world,
        const PathIndex &paths,
        const Camera &camera,
        GridExtent extent)
    {
        SceneSnapshot snapshot{
            .camera = camera,
            .extent = extent,
            .paths = {},
            .walkers = {},
            .buildings = {},
            .ghost = {}};

        snapshot.paths.assign(paths.cells().begin(), paths.cells().end());

        for (const auto entity : world.view<Walker, Cell>())
        {
            snapshot.walkers.push_back(
                WalkerView{
                    .at = world.get<Cell>(entity),
                    .facing = world.get<Walker>(entity).facing});
        }

        for (const auto entity : world.view<Building, Cell>())
        {
            snapshot.buildings.push_back(
                BuildingView{
                    .at = world.get<Cell>(entity),
                    .kind = world.get<Building>(entity).kind});
        }

        // At most one, since GridSink keeps one entity for it.
        // A loop rather than an id.
        // The snapshot then needs no way of being told which entity.
        for (const auto entity : world.view<BuildGhost>())
        {
            snapshot.ghost = world.get<BuildGhost>(entity);
        }

        return snapshot;
        // The excluded line is the local snapshot's unwind destructor.
        // Nothing between its construction and the return throws.
    } // GCOVR_EXCL_LINE

} // namespace antwika::game
