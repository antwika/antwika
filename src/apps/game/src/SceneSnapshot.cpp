#include "antwika/game/SceneSnapshot.hpp"

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
            .camera = camera, .extent = extent, .paths = {}, .walkers = {}};

        snapshot.paths.assign(paths.cells().begin(), paths.cells().end());

        for (const auto entity : world.view<Walker, Cell>())
        {
            snapshot.walkers.push_back(
                WalkerView{
                    .at = world.get<Cell>(entity),
                    .facing = world.get<Walker>(entity).facing});
        }

        return snapshot;
        // The excluded line is the local snapshot's unwind destructor.
        // Nothing between its construction and the return throws.
    } // GCOVR_EXCL_LINE

} // namespace antwika::game
