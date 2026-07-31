#include "antwika/game/SceneSnapshot.hpp"

#include <algorithm>
#include <cstdint>
#include <vector>

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
            .ghost = {},
            .hover = {}};

        snapshot.paths.assign(paths.cells().begin(), paths.cells().end());

        for (const auto entity : world.view<Walker, Cell>())
        {
            const auto walker = world.get<Walker>(entity);

            // Walker counts down to its next step.
            // A picture wants how far through this one it already is.
            // Which is the same span read the other way round.
            // A walker that never stepped is nowhere through one.
            // So it reports zero rather than what a mid-step one would.
            // That keeps the count meaningful without reading `from`.
            const auto into =
                walker.from.has_value()
                    ? static_cast<std::uint8_t>(
                          kTicksPerStep - 1 - walker.ticksUntilStep)
                    : std::uint8_t{0};

            snapshot.walkers.push_back(
                WalkerSprite{
                    .at = world.get<Cell>(entity),
                    .facing = walker.facing,
                    .from = walker.from,
                    .ticksIntoStep = into,
                    .kind = walker.kind,
                    .carried = walker.carried});
        }

        for (const auto entity : world.view<Building, Cell>())
        {
            const auto building = world.get<Building>(entity);

            snapshot.buildings.push_back(
                BuildingSprite{
                    .at = world.get<Cell>(entity),
                    .kind = building.kind,
                    .stock = building.stock});
        }

        // **Painter's order, no longer optional.**
        // One-cell buildings could never overlap.
        // So placement order was as good as any.
        // A block drawn before what is behind it is the wrong picture.
        // Screen depth here is x + y.
        // The tie-break on x makes the order total.
        // So nothing depends on how the view walked the world.
        std::ranges::sort(
            snapshot.buildings,
            [](const BuildingSprite &left, const BuildingSprite &right)
            {
                const auto depth = left.at.x + left.at.y;
                const auto other = right.at.x + right.at.y;

                return depth != other ? depth < other
                                      : left.at.x < right.at.x;
            });

        return snapshot;
        // The excluded line is the local snapshot's unwind destructor.
        // Nothing between its construction and the return throws.
    } // GCOVR_EXCL_LINE

    std::vector<WalkerView> walkerViewsOf(const World &world)
    {
        std::vector<WalkerView> views;

        for (const auto entity : world.view<Walker, Cell>())
        {
            views.push_back(
                WalkerView{
                    .at = world.get<Cell>(entity),
                    .facing = world.get<Walker>(entity).facing});
        }

        return views;
        // The excluded line is the local vector's unwind destructor.
        // Nothing between its construction and the return throws.
    } // GCOVR_EXCL_LINE

    std::vector<BuildingView> buildingViewsOf(const World &world)
    {
        std::vector<BuildingView> views;

        for (const auto entity : world.view<Building, Cell>())
        {
            views.push_back(
                BuildingView{
                    .at = world.get<Cell>(entity),
                    .kind = world.get<Building>(entity).kind});
        }

        return views;
        // The excluded line is the local vector's unwind destructor.
        // Nothing between its construction and the return throws.
    } // GCOVR_EXCL_LINE

} // namespace antwika::game
