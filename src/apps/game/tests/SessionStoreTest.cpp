#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/ecs/World.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/game/BuildTool.hpp"
#include "antwika/game/Building.hpp"
#include "antwika/game/BuildingIndex.hpp"
#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Camera.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Footprint.hpp"
#include "antwika/game/Direction.hpp"
#include "antwika/game/Errand.hpp"
#include "antwika/game/GameState.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/Path.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/Household.hpp"
#include "antwika/game/HousingLevel.hpp"
#include "antwika/game/Production.hpp"
#include "antwika/game/SaveGame.hpp"
#include "antwika/game/SceneSnapshot.hpp"
#include "antwika/game/SessionStore.hpp"
#include "antwika/game/Walker.hpp"

namespace
{

    using antwika::ecs::World;
    using antwika::game::Camera;
    using antwika::game::Cell;
    using antwika::game::Direction;
    using antwika::game::Errand;
    using antwika::game::ErrandLeg;
    using antwika::game::Production;
    using antwika::game::Resource;
    using antwika::game::GameState;
    using antwika::game::GridExtent;
    using antwika::game::Path;
    using antwika::game::PathIndex;
    using antwika::game::SaveGame;
    using antwika::game::SessionStore;
    using antwika::game::Walker;
    using antwika::game::SavedWalker;
    using antwika::log::mocks::MockLogger;

    constexpr GridExtent kExtent{.width = 16, .height = 16};

    class SessionStoreTest : public ::testing::Test
    {
    protected:
        void layPath(Cell cell)
        {
            const auto entity = world.create();
            world.add<Cell>(entity, cell);
            world.add<Path>(entity, Path{});
            paths.insert(cell);
        }

        [[nodiscard]] antwika::game::SceneSnapshot frame()
        {
            world.commit();
            return snapshotOf(world, paths, camera, kExtent);
        }

        ::testing::NiceMock<MockLogger> logger;
        World world{logger};
        PathIndex paths;
        antwika::game::BuildingIndex built;
        Camera camera;
        GameState state;
        SessionStore store{world, paths, built, camera, state, kExtent, 42};
    };

    TEST_F(SessionStoreTest, Take_HoldsTheGridTheCameraAndTheState)
    {
        layPath(Cell{.x = 1, .y = 2});
        state.score = 7;
        camera.zoomIn();
        world.commit();

        const SaveGame save = store.take();

        EXPECT_EQ(save.paths, (std::vector<Cell>{{.x = 1, .y = 2}}));
        EXPECT_EQ(save.camera, camera);
        EXPECT_EQ(save.state.score, 7U);
        EXPECT_EQ(save.extent, kExtent);
        EXPECT_EQ(save.seed, 42U);
    }

    TEST_F(SessionStoreTest, Restore_PutsBackThePathsWalkersCameraAndState)
    {
        SaveGame save;
        save.paths = {{.x = 3, .y = 3}, {.x = 4, .y = 3}};
        save.walkers = {
            {.at = {.x = 3, .y = 3}, .facing = Direction::South}};
        save.camera = Camera(antwika::gfx::Point{.x = 9, .y = 9}, 1);
        save.state = GameState{.ticksProcessed = 5, .score = 3};

        store.restore(save);

        const auto after = frame();

        EXPECT_EQ(after.paths, save.paths);
        ASSERT_EQ(after.walkers.size(), 1U);
        EXPECT_EQ(after.walkers[0].at, (Cell{.x = 3, .y = 3}));
        EXPECT_EQ(after.walkers[0].facing, Direction::South);
        EXPECT_EQ(camera, save.camera);
        EXPECT_EQ(state, save.state);
        EXPECT_TRUE(paths.has(Cell{.x = 4, .y = 3}));
    }

    TEST_F(SessionStoreTest, Restore_DestroysWhatWasAlreadyOnTheGrid)
    {
        layPath(Cell{.x = 8, .y = 8});

        const auto house = world.create();
        world.add<Cell>(house, Cell{.x = 7, .y = 7});
        world.add<antwika::game::Building>(
            house,
            antwika::game::Building{
                .kind = antwika::game::BuildingKind::House});
        world.commit();

        SaveGame save;
        save.paths = {{.x = 1, .y = 1}};

        store.restore(save);

        const auto after = frame();

        EXPECT_EQ(after.paths, save.paths);
        EXPECT_FALSE(paths.has(Cell{.x = 8, .y = 8}));
        EXPECT_EQ(world.view<Path>().size(), 1U);
        EXPECT_TRUE(after.buildings.empty());
    }

    TEST_F(SessionStoreTest, Restore_ThenTake_ComesBackTheSame)
    {
        SaveGame save;
        save.paths = {{.x = 2, .y = 2}};
        save.walkers = {
            {.at = {.x = 2, .y = 2}, .facing = Direction::West}};
        save.camera = Camera(antwika::gfx::Point{.x = 4, .y = 5}, 2);
        save.state = GameState{.ticksProcessed = 9, .score = 1};
        save.extent = kExtent;
        save.seed = 42;

        store.restore(save);
        world.commit();

        EXPECT_EQ(store.take(), save);
    }

    TEST_F(SessionStoreTest, Restore_LeavesEveryWalkerReadyToStep)
    {
        SaveGame save;
        save.walkers = {
            {.at = {.x = 0, .y = 0}, .facing = Direction::East}};

        store.restore(save);
        world.commit();

        ASSERT_EQ(world.view<Walker>().size(), 1U);

        for (const auto entity : world.view<Walker>())
        {
            EXPECT_EQ(world.get<Walker>(entity).ticksUntilStep, 0U);
        }
    }

}

namespace
{
    TEST_F(SessionStoreTest, Restore_TiesABuildingBackToItsWalker)
    {
        const auto source = world.create();
        world.add<Cell>(source, Cell{.x = 4, .y = 4});

        const auto walker = world.create();
        world.add<Cell>(walker, Cell{.x = 1, .y = 1});
        world.add<antwika::game::Walker>(walker, antwika::game::Walker{});

        world.add<antwika::game::Building>(
            source, antwika::game::Building{.walkers = {walker}});
        world.commit();

        const auto saved = store.take();
        ASSERT_EQ(saved.buildings.size(), 1U);
        ASSERT_EQ(saved.walkers.size(), 1U);
        ASSERT_EQ(
            saved.buildings[0].walkers, (std::vector<std::size_t>{0U}));

        store.restore(saved);
        world.commit();

        antwika::ecs::Entity restoredBuilding{};
        for (const auto entity : world.view<antwika::game::Building>())
        {
            restoredBuilding = entity;
        }

        const auto out = world.get<antwika::game::Building>(
            restoredBuilding).walkers[0];

        ASSERT_TRUE(world.alive(out));
        EXPECT_EQ(
            world.get<antwika::game::Walker>(out).home, restoredBuilding);
    }

    TEST_F(SessionStoreTest, Restore_PutsBackABuildingsStockAndCountdowns)
    {
        const auto source = world.create();
        world.add<Cell>(source, Cell{.x = 4, .y = 4});
        world.add<antwika::game::Building>(
            source,
            antwika::game::Building{
                .kind = antwika::game::BuildingKind::Well,
                .stock = {11, 22},
                .fireRisk = 33,
                .collapseRisk = 21,
                .ticksUntilSpawn = 44,
                .ticksUntilDrain = 55,
                .ticksUntilRisk = 66});
        world.commit();

        const auto saved = store.take();
        store.restore(saved);
        world.commit();

        ASSERT_EQ(world.view<antwika::game::Building>().size(), 1U);

        for (const auto entity : world.view<antwika::game::Building>())
        {
            const auto building = world.get<antwika::game::Building>(entity);

            EXPECT_EQ(
                building.kind, antwika::game::BuildingKind::Well);
            EXPECT_EQ(building.stock[0], 11);
            EXPECT_EQ(building.stock[1], 22);
            EXPECT_EQ(building.fireRisk, 33);
            EXPECT_EQ(building.collapseRisk, 21);
            EXPECT_EQ(building.ticksUntilSpawn, 44);
            EXPECT_EQ(building.ticksUntilDrain, 55);
            EXPECT_EQ(building.ticksUntilRisk, 66);
        }
    }

    TEST_F(SessionStoreTest, RestoreThenTake_BringsBackWhatAMarketSellsNext)
    {
        const auto source = world.create();
        world.add<Cell>(source, Cell{.x = 4, .y = 4});
        world.add<antwika::game::Building>(
            source,
            antwika::game::Building{
                .kind = antwika::game::BuildingKind::Market,
                .selling = Resource::Pottery});
        world.commit();

        const auto saved = store.take();
        store.restore(saved);
        world.commit();

        ASSERT_EQ(world.view<antwika::game::Building>().size(), 1U);

        for (const auto entity : world.view<antwika::game::Building>())
        {
            EXPECT_EQ(
                world.get<antwika::game::Building>(entity).selling,
                Resource::Pottery);
        }
    }

    TEST_F(SessionStoreTest, Restore_RebuildsTheOccupancyIndex)
    {
        const auto source = world.create();
        world.add<Cell>(source, Cell{.x = 4, .y = 4});
        world.add<antwika::game::Building>(
            source, antwika::game::Building{});
        world.commit();
        built.insert(Cell{.x = 4, .y = 4}, antwika::game::Footprint{});
        built.insert(Cell{.x = 9, .y = 9}, antwika::game::Footprint{});

        const auto saved = store.take();
        store.restore(saved);

        EXPECT_TRUE(built.has(Cell{.x = 4, .y = 4}));
        EXPECT_FALSE(built.has(Cell{.x = 9, .y = 9}));
    }

    TEST_F(SessionStoreTest, RestoreThenTake_BringsCoverageBack)
    {
        SaveGame save;
        save.buildings = {
            antwika::game::SavedBuilding{
                .at = {.x = 5, .y = 5},
                .kind = antwika::game::BuildingKind::House,
                .coverage = {3, 9}}};

        store.restore(save);
        world.commit();

        EXPECT_EQ(store.take().buildings, save.buildings);
    }

    TEST_F(SessionStoreTest, RestoreThenTake_BringsAHouseholdBack)
    {
        SaveGame save;
        save.buildings = {
            antwika::game::SavedBuilding{
                .at = {.x = 5, .y = 5},
                .kind = antwika::game::BuildingKind::House,
                .household =
                    antwika::game::Household{
                        .level = antwika::game::HousingLevel::Hovel,
                        .ticksUntilEvolve = 6,
                        .ticksUntilDevolve = 8,
                        .population = 2}},
            antwika::game::SavedBuilding{
                .at = {.x = 9, .y = 9},
                .kind = antwika::game::BuildingKind::Well}};

        store.restore(save);
        world.commit();

        EXPECT_EQ(store.take().buildings, save.buildings);
    }

    TEST_F(SessionStoreTest, RestoreThenTake_BringsTheLedgersBack)
    {
        SaveGame save;
        save.buildings = {
            antwika::game::SavedBuilding{
                .at = {.x = 5, .y = 5},
                .kind = antwika::game::BuildingKind::Farm,
                .staff = antwika::game::StoredStaff{
                    .entries = {antwika::game::StoredStaffEntry{
                        .house = 1, .count = 3}},
                    .ticksUntilDecay = 5},
                .employment = antwika::game::StoredEmployment{
                    .jobs = {}, .ticksUntilDispatch = 2}},
            antwika::game::SavedBuilding{
                .at = {.x = 9, .y = 9},
                .kind = antwika::game::BuildingKind::House,
                .employment = antwika::game::StoredEmployment{
                    .jobs = {antwika::game::StoredJob{
                        .workplace = 0, .count = 3}},
                    .ticksUntilDispatch = 7}}};

        store.restore(save);
        world.commit();

        EXPECT_EQ(store.take().buildings, save.buildings);
    }

    TEST_F(SessionStoreTest, Restore_PutsBackAnErrandAndACountdown)
    {
        SaveGame save;
        save.paths = {{.x = 3, .y = 3}};
        save.walkers = {SavedWalker{
            .at = {.x = 3, .y = 3},
            .kind = antwika::game::WalkerKind::CartPusher,
            .home = 0U,
            .errand =
                antwika::game::SavedErrand{
                    .destination = 1U,
                    .carrying = Resource::Clay,
                    .leg = ErrandLeg::Returning}}};
        save.buildings = {
            antwika::game::SavedBuilding{
                .at = {.x = 5, .y = 5},
                .kind = antwika::game::BuildingKind::ClayPit,
                .walkers = {0U},
                .ticksUntilOutput = 6},
            antwika::game::SavedBuilding{
                .at = {.x = 9, .y = 9},
                .kind = antwika::game::BuildingKind::Storage}};

        store.restore(save);
        world.commit();

        EXPECT_EQ(store.take().walkers[0].errand, save.walkers[0].errand);
        EXPECT_EQ(
            store.take().buildings[0].ticksUntilOutput,
            save.buildings[0].ticksUntilOutput);
        EXPECT_FALSE(
            store.take().buildings[1].ticksUntilOutput.has_value());
    }

    TEST_F(SessionStoreTest, Restore_PutsBackAJourneyEitherWayItGoes)
    {
        SaveGame save;
        save.paths = {{.x = 3, .y = 3}};
        save.walkers = {
            SavedWalker{
                .at = {.x = 3, .y = 3},
                .kind = antwika::game::WalkerKind::Migrant,
                .journey =
                    antwika::game::SavedJourney{
                        .towards = {.x = 5, .y = 5}, .house = 0U}},
            SavedWalker{
                .at = {.x = 3, .y = 3},
                .kind = antwika::game::WalkerKind::Migrant,
                .journey = antwika::game::SavedJourney{
                    .towards = {.x = 0, .y = 3}}}};
        save.buildings = {antwika::game::SavedBuilding{
            .at = {.x = 5, .y = 5},
            .kind = antwika::game::BuildingKind::House}};

        store.restore(save);
        world.commit();

        const auto taken = store.take();

        EXPECT_EQ(taken.walkers[0].journey, save.walkers[0].journey);
        EXPECT_EQ(taken.walkers[1].journey, save.walkers[1].journey);
    }

    TEST_F(SessionStoreTest, Restore_LeavesAWalkerWithNoJourneyWhereItIs)
    {
        SaveGame save;
        save.paths = {{.x = 3, .y = 3}};
        save.walkers = {SavedWalker{.at = {.x = 3, .y = 3}}};

        store.restore(save);
        world.commit();

        EXPECT_FALSE(store.take().walkers[0].journey.has_value());
    }

    TEST_F(SessionStoreTest, Restore_LeavesAWalkerWithNoErrandRoaming)
    {
        SaveGame save;
        save.paths = {{.x = 3, .y = 3}};
        save.walkers = {SavedWalker{.at = {.x = 3, .y = 3}}};

        store.restore(save);
        world.commit();

        EXPECT_FALSE(store.take().walkers[0].errand.has_value());
    }

}

    TEST_F(SessionStoreTest, RestoreThenTake_BringsTheRuinsBack)
    {
        SaveGame save;
        save.extent = kExtent;
        save.ruins = {antwika::game::SavedRuin{
            .at = Cell{.x = 4, .y = 4},
            .kind = antwika::game::BuildingKind::Farm,
            .state = antwika::game::RuinState::Burning,
            .ticksUntilOut = 99}};
        save.walkers = {SavedWalker{
            .at = Cell{.x = 1, .y = 1},
            .kind = antwika::game::WalkerKind::Fireman,
            .fireCall = 0U}};

        store.restore(save);
        world.commit();

        EXPECT_TRUE(built.has(Cell{.x = 4, .y = 4}));
        EXPECT_TRUE(built.has(Cell{.x = 5, .y = 5}));

        const auto taken = store.take();

        EXPECT_EQ(taken.ruins, save.ruins);
        ASSERT_EQ(taken.walkers.size(), 1U);
        EXPECT_EQ(taken.walkers[0].fireCall, 0U);
    }
