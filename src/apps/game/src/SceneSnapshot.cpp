#include "antwika/game/SceneSnapshot.hpp"

#include <algorithm>
#include <cstdint>
#include <vector>

#include "antwika/game/Building.hpp"
#include "antwika/game/Coverage.hpp"
#include "antwika/game/Employment.hpp"
#include "antwika/game/Staff.hpp"
#include "antwika/game/HousingQuery.hpp"
#include "antwika/game/Walker.hpp"

namespace antwika::game
{
    namespace
    {
        // One number for both readings -- see BuildingView::employed.
        [[nodiscard]] std::int32_t employedOf(
            const World &world, antwika::ecs::Entity entity)
        {
            if (world.has<Employment>(entity))
            {
                return employedCount(world.get<Employment>(entity));
            }

            if (world.has<Staff>(entity))
            {
                return staffCount(world.get<Staff>(entity));
            }

            return 0;
        }
    } // namespace


    SceneSnapshot snapshotOf(
        const World &world,
        const PathIndex &paths,
        const Camera &camera,
        GridExtent extent,
        bool paused)
    {
        // Built empty and filled in rather than named member by member.
        // Whatever this function does not answer for is already default.
        // There are four such members now.
        // Naming each of them as {} says nothing and dates immediately.
        SceneSnapshot snapshot;
        snapshot.camera = camera;
        snapshot.extent = extent;
        snapshot.paused = paused;

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

            // Read out here rather than inside the record below.
            // Two calls inside one aggregate need an unwind pad.
            // To destroy the half-built record they were made in.
            // Which is a landing pad on a line nothing reaches.
            const auto coverage = coverageOf(world, entity).ticksLeft;
            const auto level = levelOf(world, entity);
            const auto living = populationAt(world, entity);

            snapshot.buildings.push_back(
                BuildingSprite{
                    .at = world.get<Cell>(entity),
                    .kind = building.kind,
                    .stock = building.stock,
                    .coverage = coverage,
                    .level = level,
                    .population = living,
                    .employed = employedOf(world, entity)});
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
            // Read out here for the reason snapshotOf()'s pair is.
            const auto coverage = coverageOf(world, entity).ticksLeft;
            const auto level = levelOf(world, entity);
            const auto living = populationAt(world, entity);

            views.push_back(
                BuildingView{
                    .at = world.get<Cell>(entity),
                    .kind = world.get<Building>(entity).kind,
                    .coverage = coverage,
                    .level = level,
                    .population = living,
                    .employed = employedOf(world, entity)});
        }

        return views;
        // The excluded line is the local vector's unwind destructor.
        // Nothing between its construction and the return throws.
    } // GCOVR_EXCL_LINE

} // namespace antwika::game
