#include "antwika/game/SceneSnapshot.hpp"

#include <algorithm>

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
            .buildings = {}};

        snapshot.paths.assign(paths.cells().begin(), paths.cells().end());

        for (const auto entity : world.view<Walker, Cell>())
        {
            const auto &walker = world.get<Walker>(entity);

            snapshot.walkers.push_back(
                WalkerView{
                    .at = world.get<Cell>(entity),
                    .facing = walker.facing,
                    .kind = walker.kind,
                    .carried = walker.carried});
        }

        for (const auto entity : world.view<Building, Cell>())
        {
            const auto &building = world.get<Building>(entity);

            snapshot.buildings.push_back(
                BuildingView{
                    .at = world.get<Cell>(entity),
                    .kind = building.kind,
                    .held = building.stock.held,
                    .capacity = building.stock.capacity});
        }

        // Ascending by cell, which is what lets a scene binary-search it.
        // Only one building ever stands on a cell, so the order is total.
        std::ranges::sort(
            snapshot.buildings,
            [](const BuildingView &one, const BuildingView &other)
            { return one.at < other.at; });

        return snapshot;
        // The excluded line is the local snapshot's unwind destructor.
        // Nothing between its construction and the return throws.
    } // GCOVR_EXCL_LINE

} // namespace antwika::game
