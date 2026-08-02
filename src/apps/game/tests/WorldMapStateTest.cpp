#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>

#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/game/Building.hpp"
#include "antwika/game/BuildingIndex.hpp"
#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Camera.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Footprint.hpp"
#include "antwika/game/LiveGrid.hpp"
#include "antwika/game/Path.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/Walker.hpp"
#include "antwika/game/WorldMap.hpp"
#include "antwika/game/WorldMapError.hpp"
#include "antwika/game/WorldMapState.hpp"

namespace
{

    using antwika::ecs::Entity;
    using antwika::ecs::World;
    using antwika::game::Building;
    using antwika::game::BuildingIndex;
    using antwika::game::BuildingKind;
    using antwika::game::Camera;
    using antwika::game::Cell;
    using antwika::game::generateWorldMap;
    using antwika::game::kCityCount;
    using antwika::game::LiveGrid;
    using antwika::game::Path;
    using antwika::game::PathIndex;
    using antwika::game::Walker;
    using antwika::game::WorldMapError;
    using antwika::game::WorldMapState;
    using antwika::log::mocks::MockLogger;

    class WorldMapStateTest : public ::testing::Test
    {
    protected:
        // What GridSink does on a click, minus the click.
        void layPath(Cell cell)
        {
            const auto entity = world.create();
            world.add<Cell>(entity, cell);
            world.add<Path>(entity, Path{});
            paths.insert(cell);
        }

        Entity putUp(Cell cell, BuildingKind kind)
        {
            const auto entity = world.create();
            world.add<Cell>(entity, cell);
            world.add<Building>(entity, Building{.kind = kind});
            (void)built.insert(cell, antwika::game::footprintOf(kind));
            return entity;
        }

        void dropWalker(Cell cell)
        {
            const auto entity = world.create();
            world.add<Cell>(entity, cell);
            world.add<Walker>(entity, Walker{});
        }

        [[nodiscard]] std::size_t buildingsStanding()
        {
            world.commit();
            return world.view<Building, Cell>().size();
        }

        [[nodiscard]] std::size_t walkersStanding()
        {
            world.commit();
            return world.view<Walker, Cell>().size();
        }

        ::testing::NiceMock<MockLogger> logger;
        World world{logger};
        PathIndex paths;
        BuildingIndex built;
        Camera camera;
        LiveGrid live{
            .world = world,
            .paths = paths,
            .built = built,
            .camera = camera};
        WorldMapState state{generateWorldMap({12, 10, 99})};
    };

    TEST_F(WorldMapStateTest, StartsWithItsFirstCityOpen)
    {
        EXPECT_TRUE(state.cityOpen());
        EXPECT_EQ(state.city(), 0U);
        EXPECT_EQ(state.world().width, 12U);
    }

    TEST_F(WorldMapStateTest, OpeningACitySwapsItsGridIn)
    {
        state.cityPaths(2).insert(Cell{7, 8});

        state.openCityAt(2, live);

        EXPECT_TRUE(state.cityOpen());
        EXPECT_EQ(state.city(), 2U);
        EXPECT_TRUE(paths.has(Cell{7, 8}));
    }

    TEST_F(WorldMapStateTest, ClosingKeepsTheGridWithTheCityItBelongsTo)
    {
        state.openCityAt(1, live);
        paths.insert(Cell{3, 4});
        camera.zoomIn();
        state.closeCity(live);

        EXPECT_FALSE(state.cityOpen());
        EXPECT_TRUE(state.cityPaths(1).has(Cell{3, 4}));
        EXPECT_EQ(state.cityCamera(1), camera);

        // A second close has nothing left to put away.
        // So a stray press on the way-back key is a no-op.
        paths = PathIndex{};
        state.closeCity(live);
        EXPECT_TRUE(state.cityPaths(1).has(Cell{3, 4}));
    }

    TEST_F(WorldMapStateTest, LeavingACityAndComingBackShowsWhatWasBuilt)
    {
        state.openCityAt(0, live);
        paths.insert(Cell{1, 1});

        state.openCityAt(3, live);
        EXPECT_FALSE(paths.has(Cell{1, 1}));

        paths.insert(Cell{9, 9});

        state.openCityAt(0, live);
        EXPECT_TRUE(paths.has(Cell{1, 1}));
        EXPECT_FALSE(paths.has(Cell{9, 9}));
    }

    // The bug this class exists to make impossible.
    // The buildings and the walkers lived in one World nothing swapped.
    // So every city showed every other city's.
    TEST_F(WorldMapStateTest, BuildingsAndWalkersBelongToOneCityOnly)
    {
        layPath(Cell{2, 2});
        putUp(Cell{4, 4}, BuildingKind::House);
        putUp(Cell{6, 6}, BuildingKind::Farm);
        dropWalker(Cell{2, 2});
        world.commit();

        state.openCityAt(1, live);

        EXPECT_EQ(buildingsStanding(), 0U);
        EXPECT_EQ(walkersStanding(), 0U);
        EXPECT_EQ(built.size(), 0U);
        EXPECT_EQ(paths.size(), 0U);

        // Not merely cleared: what was there is still city 0's.
        state.openCityAt(0, live);

        EXPECT_EQ(buildingsStanding(), 2U);
        EXPECT_EQ(walkersStanding(), 1U);
        EXPECT_TRUE(built.has(Cell{4, 4}));
        EXPECT_TRUE(built.has(Cell{6, 6}));
        EXPECT_TRUE(paths.has(Cell{2, 2}));
    }

    TEST_F(WorldMapStateTest, ClosingKeepsWhatThisTickHasOnlyStaged)
    {
        // create() is immediate where add() is staged.
        // So this building is not visible until a commit.
        // Leaving the city must not be what loses it.
        putUp(Cell{5, 5}, BuildingKind::House);

        state.closeCity(live);
        state.openCityAt(0, live);

        EXPECT_EQ(buildingsStanding(), 1U);
    }

    TEST_F(WorldMapStateTest, ABuildingKeepsTheWalkerItHasOut)
    {
        const auto home = putUp(Cell{4, 4}, BuildingKind::Farm);
        dropWalker(Cell{5, 4});
        world.commit();

        auto sent = world.get<Building>(home);
        sent.walkers[0] = *world.view<Walker, Cell>().begin();
        world.set<Building>(home, sent);
        world.commit();

        state.openCityAt(1, live);
        state.openCityAt(0, live);
        world.commit();

        const auto building = *world.view<Building, Cell>().begin();
        const auto walker = *world.view<Walker, Cell>().begin();

        // Every entity was recreated, so a stored handle would be stale.
        EXPECT_EQ(world.get<Building>(building).walkers[0], walker);
        EXPECT_EQ(world.get<Walker>(walker).home, building);
    }

    TEST_F(WorldMapStateTest, EachCityKeepsItsOwnGrid)
    {
        state.cityPaths(0).insert(Cell{3, 4});
        state.cityCamera(0).zoomIn();

        EXPECT_TRUE(state.cityPaths(0).has(Cell{3, 4}));
        EXPECT_FALSE(state.cityPaths(1).has(Cell{3, 4}));

        const WorldMapState &readOnly = state;
        EXPECT_EQ(readOnly.cityPaths(0).size(), 1U);
        EXPECT_NE(
            readOnly.cityCamera(0).zoomLevel(),
            readOnly.cityCamera(1).zoomLevel());
    }

    TEST_F(WorldMapStateTest, NoSuchCityIsRefusedEverywhere)
    {
        const WorldMapState &readOnly = state;

        EXPECT_THROW(state.openCityAt(kCityCount, live), WorldMapError);
        EXPECT_THROW((void)state.cityPaths(kCityCount), WorldMapError);
        EXPECT_THROW((void)state.cityCamera(kCityCount), WorldMapError);
        EXPECT_THROW((void)readOnly.cityPaths(kCityCount), WorldMapError);
        EXPECT_THROW((void)readOnly.cityCamera(kCityCount), WorldMapError);
    }

} // namespace
