#include "antwika/game/SceneSnapshot.hpp"

#include <algorithm>
#include <cstdint>
#include <vector>

#include "antwika/game/Building.hpp"
#include "antwika/game/Coverage.hpp"
#include "antwika/game/Employment.hpp"
#include "antwika/game/Errand.hpp"
#include "antwika/game/Staff.hpp"
#include "antwika/game/HousingQuery.hpp"
#include "antwika/game/Ruin.hpp"
#include "antwika/game/Walker.hpp"

namespace antwika::game
{
    namespace
    {
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
    }

    SceneSnapshot snapshotOf(
        const World &world,
        const PathIndex &paths,
        const Camera &camera,
        GridExtent extent,
        bool paused)
    {
        SceneSnapshot snapshot;
        snapshot.camera = camera;
        snapshot.extent = extent;
        snapshot.paused = paused;

        snapshot.paths.assign(paths.cells().begin(), paths.cells().end());

        for (const auto entity : world.view<Walker, Cell>())
        {
            const auto walker = world.get<Walker>(entity);

            const auto into =
                walker.from.has_value()
                    ? static_cast<std::uint8_t>(
                          kTicksPerStep - 1 - walker.ticksUntilStep)
                    : std::uint8_t{0};

            const auto carrying = world.has<Errand>(entity)
                ? std::optional<Resource>(
                      world.get<Errand>(entity).carrying)
                : std::nullopt;

            snapshot.walkers.push_back(
                WalkerSprite{
                    .at = world.get<Cell>(entity),
                    .facing = walker.facing,
                    .from = walker.from,
                    .ticksIntoStep = into,
                    .kind = walker.kind,
                    .carried = walker.carried,
                    .carrying = carrying});
        }

        for (const auto entity : world.view<Building, Cell>())
        {
            const auto building = world.get<Building>(entity);

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
                    .employed = employedOf(world, entity),
                    .fireRisk = building.fireRisk,
                    .collapseRisk = building.collapseRisk,
                    .diseaseRisk = building.diseaseRisk});
        }

        for (const auto entity : world.view<Ruin, Cell>())
        {
            const auto ruin = world.get<Ruin>(entity);

            snapshot.ruins.push_back(
                RuinView{
                    .at = world.get<Cell>(entity),
                    .kind = ruin.kind,
                    .state = ruin.state});
        }

        std::ranges::sort(
            snapshot.buildings,
            [](const BuildingSprite &left, const BuildingSprite &right)
            {
                const auto depth = left.at.x + left.at.y;
                const auto other = right.at.x + right.at.y;

                return depth != other ? depth < other
                                      : left.at.x < right.at.x;
            });

        std::ranges::sort(
            snapshot.ruins,
            [](const RuinView &left, const RuinView &right)
            {
                const auto depth = left.at.x + left.at.y;
                const auto other = right.at.x + right.at.y;

                return depth != other ? depth < other
                                      : left.at.x < right.at.x;
            });

        return snapshot;
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
    } // GCOVR_EXCL_LINE

    std::vector<BuildingView> buildingViewsOf(const World &world)
    {
        std::vector<BuildingView> views;

        for (const auto entity : world.view<Building, Cell>())
        {
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
    } // GCOVR_EXCL_LINE

    std::vector<RuinView> ruinViewsOf(const World &world)
    {
        std::vector<RuinView> views;

        for (const auto entity : world.view<Ruin, Cell>())
        {
            const auto ruin = world.get<Ruin>(entity);

            views.push_back(
                RuinView{
                    .at = world.get<Cell>(entity),
                    .kind = ruin.kind,
                    .state = ruin.state});
        }

        return views;
    } // GCOVR_EXCL_LINE

}
