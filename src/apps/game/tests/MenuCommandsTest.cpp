#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/ecs/World.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/game/AppMode.hpp"
#include "antwika/game/BuildingIndex.hpp"
#include "antwika/game/Camera.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/GameState.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/LiveGrid.hpp"
#include "antwika/game/MenuCommands.hpp"
#include "antwika/game/Path.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/SaveGame.hpp"
#include "antwika/game/SessionStore.hpp"
#include "antwika/game/WorldMap.hpp"
#include "antwika/game/WorldMapState.hpp"

using antwika::game::AppMode;
using antwika::game::AppModeState;
using antwika::game::Camera;
using antwika::game::Cell;
using antwika::game::GameConfig;
using antwika::game::GameState;
using antwika::game::GridExtent;
using antwika::game::LiveGrid;
using antwika::game::MenuCommands;
using antwika::game::PathIndex;
using antwika::game::SessionStore;
using antwika::game::WorldMapState;
using antwika::gfx::Point;
using antwika::log::mocks::MockLogger;
using ::testing::NiceMock;

namespace
{
    constexpr GridExtent kExtent{.width = 16, .height = 16};
    constexpr Point kHome{.x = 512, .y = 48};

    class MenuCommandsTest : public ::testing::Test
    {
    protected:
        void layRoad(Cell cell)
        {
            const auto entity = world.create();
            world.add<Cell>(entity, cell);
            world.add<antwika::game::Path>(entity, antwika::game::Path{});
            paths.insert(cell);
            world.commit();
        }

        NiceMock<MockLogger> logger;
        antwika::ecs::World world{logger};
        PathIndex paths;
        antwika::game::BuildingIndex built;
        Camera camera{kHome};
        GameState state;

        AppModeState mode{AppMode::CityMap};
        WorldMapState cities{antwika::game::WorldMap{}};
        SessionStore session{
            world, paths, built, camera, state, kExtent, 7};
        LiveGrid live{
            .world = world,
            .paths = paths,
            .built = built,
            .camera = camera};
        MenuCommands commands{
            mode, session, cities, live, Camera{kHome}, GameConfig{}};
    };
}

TEST_F(MenuCommandsTest, NewGame_EmptiesTheGridItIsOn)
{
    layRoad(Cell{.x = 3, .y = 4});
    ASSERT_TRUE(paths.has(Cell{.x = 3, .y = 4}));

    commands.newGame();
    world.commit();

    EXPECT_FALSE(paths.has(Cell{.x = 3, .y = 4}));
}

TEST_F(MenuCommandsTest, NewGame_PutsTheCameraBackWhereARunStarts)
{
    camera.panBy(100, 100);
    camera.zoomOut();

    commands.newGame();

    EXPECT_EQ(Camera{kHome}, camera);
}

TEST_F(MenuCommandsTest, NewGame_ResetsTheStateTheReducerFolds)
{
    state = GameState{.ticksProcessed = 9, .score = 42};

    commands.newGame();

    EXPECT_EQ(GameState{}, state);
}

TEST_F(MenuCommandsTest, NewGame_StaysOnTheCity)
{
    commands.newGame();

    EXPECT_EQ(AppMode::CityMap, mode.next());
}

TEST_F(MenuCommandsTest, OpenSaves_AsksForThePicker)
{
    commands.openSaves();

    EXPECT_EQ(AppMode::SaveLoad, mode.next());
}

TEST_F(MenuCommandsTest, MainMenu_AsksForTheMainMenu)
{
    commands.mainMenu();

    EXPECT_EQ(AppMode::MainMenu, mode.next());
}

TEST_F(MenuCommandsTest, WorldMap_AsksForTheWorldMap)
{
    commands.worldMap();

    EXPECT_EQ(AppMode::WorldMap, mode.next());
}

TEST_F(MenuCommandsTest, WorldMap_PutsTheOpenCityAway)
{
    layRoad(Cell{.x = 2, .y = 2});

    commands.worldMap();

    EXPECT_FALSE(cities.cityOpen());
    EXPECT_TRUE(cities.cityPaths(0).has(Cell{.x = 2, .y = 2}));
}

TEST_F(MenuCommandsTest, Request_LandsAtTheTickBoundaryAndNotBefore)
{
    commands.mainMenu();

    EXPECT_EQ(AppMode::CityMap, mode.mode());
    EXPECT_EQ(AppMode::MainMenu, mode.next());
}

TEST_F(MenuCommandsTest, NewGame_OpensTheBankWithTheConfiguredFunds)
{
    GameConfig config;
    config.startingMoney = 1234;
    MenuCommands tuned{mode, session, cities, live, Camera{kHome}, config};

    tuned.newGame();

    EXPECT_EQ(state.money, 1234);
}
