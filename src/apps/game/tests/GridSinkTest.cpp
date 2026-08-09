#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/ecs/SystemScheduler.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/game/Building.hpp"
#include "antwika/game/BuildingIndex.hpp"
#include "antwika/game/BuildTool.hpp"
#include "antwika/game/Camera.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Cost.hpp"
#include "antwika/game/Coverage.hpp"
#include "antwika/game/GameState.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/GridSink.hpp"
#include "antwika/game/InputFold.hpp"
#include "antwika/game/IsoProjection.hpp"
#include "antwika/game/LiveGrid.hpp"
#include "antwika/game/Path.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/RoadDrag.hpp"
#include "antwika/game/Ruin.hpp"
#include "antwika/game/Service.hpp"
#include "antwika/game/UiOverlay.hpp"
#include "antwika/game/Walker.hpp"
#include "antwika/game/WorldMapState.hpp"

using antwika::ecs::SystemScheduler;
using antwika::ecs::World;
using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::game::BuildingIndex;
using antwika::game::BuildTool;
using antwika::game::Camera;
using antwika::game::Cell;
using antwika::game::cellCentre;
using antwika::game::costOf;
using antwika::game::GameConfig;
using antwika::game::GameState;
using antwika::game::GridExtent;
using antwika::game::GridSink;
using antwika::game::InputFold;
using antwika::game::kRoadCost;
using antwika::game::kStartingMoney;
using antwika::game::LiveGrid;
using antwika::game::Path;
using antwika::game::PathIndex;
using antwika::game::RoadDrag;
using antwika::game::UiOverlay;
using antwika::game::Walker;
using antwika::game::WorldMap;
using antwika::game::WorldMapState;
using antwika::input::InputEvent;
using antwika::input::InputEventCodec;
using antwika::input::MouseButton;
using antwika::input::PointerButtonPressed;
using antwika::input::PointerButtonReleased;
using antwika::input::PointerMoved;
using antwika::input::PointerScrolled;
using antwika::input::Position;
using antwika::log::mocks::MockLogger;

namespace
{
    constexpr GridExtent kExtent{.width = 16, .height = 16};

    class GridSinkTest : public ::testing::Test
    {
    protected:
        [[nodiscard]] Position pixelOf(Cell cell) const
        {
            const auto point = cellCentre(cell, camera);
            return Position{.x = point.x, .y = point.y};
        }

        void dispatch(const TickEvent &event)
        {
            input.handle(event);
            sink.handle(event);
        }

        void send(const InputEvent &event)
        {
            dispatch(TickEvent{.tick = 0, .event = codec.encode(event)});
        }

        void pressAt(Cell cell, MouseButton button)
        {
            send(
                PointerButtonPressed{
                    .button = button, .position = pixelOf(cell)});
        }

        void clickAt(Cell cell, MouseButton button)
        {
            pressAt(cell, button);
            send(
                PointerButtonReleased{
                    .button = button, .position = pixelOf(cell)});
        }

        void tick()
        {
            dispatch(
                TickEvent{
                    .tick = 0,
                    .event = Event{
                        .name = antwika::engine::events::kTick}});
        }

        [[nodiscard]] std::size_t walkerCount()
        {
            world.commit();
            return world.view<Walker, Cell>().size();
        }

        [[nodiscard]] std::size_t pathEntityCount()
        {
            world.commit();
            return world.view<Path, Cell>().size();
        }

        ::testing::NiceMock<MockLogger> logger;
        World world{logger};
        PathIndex paths;
        BuildingIndex built;
        Camera camera{antwika::gfx::Point{.x = 400, .y = 40}};
        SystemScheduler scheduler;
        InputEventCodec codec;
        InputFold input{codec};

        UiOverlay overlay;

        LiveGrid live{
            .world = world,
            .paths = paths,
            .built = built,
            .camera = camera};
        WorldMapState cities{WorldMap{}};
        RoadDrag drag;
        GameState state;
        GridSink sink{
            world,
            paths,
            camera,
            kExtent,
            scheduler,
            input,
            overlay,
            cities,
            built,
            drag,
            state,
            GameConfig{}};
    };
}

TEST_F(GridSinkTest, LeftClick_LaysAPathAtTheClickedCell)
{
    constexpr Cell target{.x = 3, .y = 4};

    clickAt(target, MouseButton::Left);

    EXPECT_TRUE(paths.has(target));
    EXPECT_EQ(paths.size(), 1U);
    EXPECT_EQ(pathEntityCount(), 1U);
}

TEST_F(GridSinkTest, LeftClick_LaysNoSecondPathOnTheSameCell)
{
    constexpr Cell target{.x = 3, .y = 4};

    clickAt(target, MouseButton::Left);
    clickAt(target, MouseButton::Left);

    EXPECT_EQ(paths.size(), 1U);
    EXPECT_EQ(pathEntityCount(), 1U);
}

TEST_F(GridSinkTest, LeftClick_LaysNothingOutsideTheExtent)
{
    for (const auto outside : {
             Cell{.x = -1, .y = 0},
             Cell{.x = 0, .y = -1},
             Cell{.x = kExtent.width, .y = 0},
             Cell{.x = 0, .y = kExtent.height},
         })
    {
        clickAt(outside, MouseButton::Left);
    }

    EXPECT_EQ(paths.size(), 0U);
    EXPECT_EQ(pathEntityCount(), 0U);
}

TEST_F(GridSinkTest, RightPress_PutsAWalkerOnAPath)
{
    constexpr Cell target{.x = 2, .y = 2};
    clickAt(target, MouseButton::Left);

    clickAt(target, MouseButton::Right);
    clickAt(target, MouseButton::Right);

    EXPECT_EQ(walkerCount(), 1U);
}

TEST_F(GridSinkTest, RightPress_PutsNoWalkerOnBareGround)
{
    clickAt(Cell{.x = 2, .y = 2}, MouseButton::Right);

    EXPECT_EQ(walkerCount(), 0U);
}

TEST_F(GridSinkTest, RightPress_LeavesBuildModeWithABuildingToolSelected)
{
    overlay.select(BuildTool::House);

    clickAt(Cell{.x = 2, .y = 2}, MouseButton::Right);

    EXPECT_FALSE(overlay.tool().has_value());
}

TEST_F(GridSinkTest, LeftClick_LaysNothingOnceThePaletteIsPutDown)
{
    constexpr Cell target{.x = 3, .y = 4};

    overlay.select(BuildTool::House);
    clickAt(target, MouseButton::Right);
    clickAt(target, MouseButton::Left);

    EXPECT_EQ(paths.size(), 0U);
    EXPECT_EQ(pathEntityCount(), 0U);
    EXPECT_EQ(built.size(), 0U);
}

TEST_F(GridSinkTest, RightPress_PlacesNoWalkerWhileLeavingBuildMode)
{
    constexpr Cell target{.x = 2, .y = 2};
    clickAt(target, MouseButton::Left);

    overlay.select(BuildTool::Farm);
    clickAt(target, MouseButton::Right);

    EXPECT_EQ(walkerCount(), 0U);

    clickAt(target, MouseButton::Right);

    EXPECT_EQ(walkerCount(), 1U);
}

TEST_F(GridSinkTest, RightPress_IsAWalkerAgainOnceBuildModeIsLeft)
{
    constexpr Cell target{.x = 2, .y = 2};
    clickAt(target, MouseButton::Left);

    overlay.select(BuildTool::EngineerPost);
    clickAt(target, MouseButton::Right);
    clickAt(target, MouseButton::Right);

    EXPECT_FALSE(overlay.tool().has_value());
    EXPECT_EQ(walkerCount(), 1U);
}

TEST_F(GridSinkTest, RightPress_PutsTheRoadBrushDown)
{
    constexpr Cell target{.x = 2, .y = 2};
    clickAt(target, MouseButton::Left);

    clickAt(target, MouseButton::Right);

    EXPECT_FALSE(overlay.tool().has_value());
    EXPECT_EQ(walkerCount(), 0U);
}

TEST_F(GridSinkTest, RightPress_LeavesBuildModeAloneWhereTheToolbarIs)
{
    overlay.select(BuildTool::House);
    overlay.set({}, {}, true);

    clickAt(Cell{.x = 2, .y = 2}, MouseButton::Right);

    EXPECT_EQ(overlay.tool(), BuildTool::House);
}

TEST_F(GridSinkTest, RightPress_LeavesBuildModeFromOutsideTheExtent)
{
    overlay.select(BuildTool::House);

    clickAt(Cell{.x = -4, .y = -4}, MouseButton::Right);

    EXPECT_FALSE(overlay.tool().has_value());
}

TEST_F(GridSinkTest, MiddlePress_LaysNothingAndPlacesNothing)
{
    clickAt(Cell{.x = 2, .y = 2}, MouseButton::Middle);

    EXPECT_EQ(paths.size(), 0U);
    EXPECT_EQ(walkerCount(), 0U);
}

TEST_F(GridSinkTest, Release_PlacesNothing)
{
    constexpr Cell target{.x = 1, .y = 1};
    clickAt(target, MouseButton::Left);

    send(
        PointerButtonReleased{
            .button = MouseButton::Left, .position = pixelOf(target)});

    EXPECT_EQ(paths.size(), 1U);
    EXPECT_EQ(pathEntityCount(), 1U);
}

TEST_F(GridSinkTest, MiddleDrag_PansTheCameraByTheMovement)
{
    const auto before = camera.pan();

    send(
        PointerButtonPressed{
            .button = MouseButton::Middle,
            .position = {.x = 100, .y = 100}});
    send(PointerMoved{.position = {.x = 130, .y = 80}});

    EXPECT_EQ(camera.pan().x, before.x + 30);
    EXPECT_EQ(camera.pan().y, before.y - 20);
}

TEST_F(GridSinkTest, MiddleDrag_SumsSuccessiveMovements)
{
    const auto before = camera.pan();

    send(
        PointerButtonPressed{
            .button = MouseButton::Middle,
            .position = {.x = 100, .y = 100}});
    send(PointerMoved{.position = {.x = 110, .y = 100}});
    send(PointerMoved{.position = {.x = 125, .y = 100}});

    EXPECT_EQ(camera.pan().x, before.x + 25);
}

TEST_F(GridSinkTest, Movement_PansNothingWithNoButtonHeld)
{
    const auto before = camera.pan();

    send(PointerMoved{.position = {.x = 100, .y = 100}});
    send(PointerMoved{.position = {.x = 300, .y = 300}});

    EXPECT_EQ(camera.pan(), before);
}

TEST_F(GridSinkTest, Movement_StopsPanningOnceTheButtonComesUp)
{
    send(
        PointerButtonPressed{
            .button = MouseButton::Middle,
            .position = {.x = 100, .y = 100}});
    send(PointerMoved{.position = {.x = 110, .y = 100}});

    const auto held = camera.pan();

    send(
        PointerButtonReleased{
            .button = MouseButton::Middle,
            .position = {.x = 110, .y = 100}});
    send(PointerMoved{.position = {.x = 400, .y = 100}});

    EXPECT_EQ(camera.pan(), held);
}

TEST_F(GridSinkTest, Scroll_ZoomsInAndOut)
{
    const auto start = camera.zoomLevel();

    send(PointerScrolled{.vertical = 1});
    EXPECT_EQ(camera.zoomLevel(), start + 1);

    send(PointerScrolled{.vertical = -1});
    EXPECT_EQ(camera.zoomLevel(), start);
}

TEST_F(GridSinkTest, Scroll_ThenAClickResolvesAgainstTheNewZoom)
{
    constexpr Cell target{.x = 5, .y = 6};

    send(PointerMoved{.position = pixelOf(target)});
    send(PointerScrolled{.vertical = 1});

    clickAt(target, MouseButton::Left);

    EXPECT_TRUE(paths.has(target));
}

TEST_F(GridSinkTest, Tick_RunsTheSchedulerAndCommitsTheWorld)
{
    constexpr Cell target{.x = 3, .y = 3};
    clickAt(target, MouseButton::Left);

    EXPECT_EQ((world.view<Path, Cell>().size()), 0U);

    tick();

    EXPECT_EQ((world.view<Path, Cell>().size()), 1U);
}

TEST_F(GridSinkTest, Tick_ClearsTheEdgesSoADragDoesNotCarryOver)
{
    send(
        PointerButtonPressed{
            .button = MouseButton::Middle,
            .position = {.x = 100, .y = 100}});
    tick();

    const auto after = camera.pan();

    send(PointerMoved{.position = {.x = 120, .y = 100}});

    EXPECT_EQ(camera.pan().x, after.x + 20);
}

TEST_F(GridSinkTest, Handle_IgnoresAnEventThatIsNotInput)
{
    dispatch(
        TickEvent{
            .tick = 0,
            .event = Event{.name = "game.score_increment"}});

    EXPECT_EQ(paths.size(), 0U);
    EXPECT_EQ(walkerCount(), 0U);
}

TEST_F(GridSinkTest, LeftClick_LaysNoPathWhereTheToolbarIs)
{
    constexpr Cell target{.x = 3, .y = 4};

    overlay.set({}, {}, true);
    clickAt(target, MouseButton::Left);

    EXPECT_FALSE(paths.has(target));
    EXPECT_EQ(paths.size(), 0U);
}

TEST_F(GridSinkTest, Scroll_DoesNotZoomWhereTheToolbarIs)
{
    const auto before = camera;

    overlay.set({}, {}, true);
    send(PointerScrolled{.vertical = 1});

    EXPECT_EQ(before, camera);
}

TEST_F(GridSinkTest, Move_KeepsPanningAcrossTheToolbar)
{
    send(
        PointerButtonPressed{
            .button = MouseButton::Middle,
            .position = Position{.x = 100, .y = 100}});

    const auto before = camera.pan();

    overlay.set({}, {}, true);
    send(PointerMoved{.position = Position{.x = 110, .y = 100}});

    EXPECT_EQ(before.x + 10, camera.pan().x);
}

TEST_F(GridSinkTest, LeftClick_LaysNothingWhileNoCityIsOpen)
{
    cities.closeCity(live);

    clickAt(Cell{.x = 3, .y = 4}, MouseButton::Left);

    EXPECT_FALSE(paths.has(Cell{.x = 3, .y = 4}));
    EXPECT_EQ(0U, pathEntityCount());
}

TEST_F(GridSinkTest, Tick_StillRunsWhileNoCityIsOpen)
{
    clickAt(Cell{.x = 3, .y = 4}, MouseButton::Left);
    cities.closeCity(live);

    tick();

    EXPECT_EQ(1U, pathEntityCount());
}

TEST_F(GridSinkTest, LeftDrag_LaysTheWholeRunOnTheRelease)
{
    pressAt(Cell{.x = 2, .y = 3}, MouseButton::Left);
    send(PointerMoved{.position = pixelOf(Cell{.x = 5, .y = 3})});
    send(
        PointerButtonReleased{
            .button = MouseButton::Left,
            .position = pixelOf(Cell{.x = 5, .y = 3})});

    for (std::int32_t x = 2; x <= 5; ++x)
    {
        EXPECT_TRUE(paths.has(Cell{.x = x, .y = 3}));
    }

    EXPECT_EQ(paths.size(), 4U);
}

TEST_F(GridSinkTest, LeftDrag_LaysNothingWhenNoRouteExists)
{
    for (std::int32_t y = 0; y < kExtent.height; ++y)
    {
        built.insert(Cell{.x = 4, .y = y}, antwika::game::Footprint{});
    }

    clickAt(Cell{.x = 2, .y = 3}, MouseButton::Left);
    send(
        PointerButtonReleased{
            .button = MouseButton::Left,
            .position = pixelOf(Cell{.x = 6, .y = 3})});

    EXPECT_EQ(paths.size(), 1U);
    EXPECT_TRUE(paths.has(Cell{.x = 2, .y = 3}));
}

TEST_F(GridSinkTest, LeftDrag_IsUnderWayFromThePressToTheRelease)
{
    pressAt(Cell{.x = 2, .y = 3}, MouseButton::Left);

    EXPECT_TRUE(drag.active());

    send(
        PointerButtonReleased{
            .button = MouseButton::Left,
            .position = pixelOf(Cell{.x = 3, .y = 3})});

    EXPECT_FALSE(drag.active());
}

TEST_F(GridSinkTest, LeftPress_StartsNoDragForABuildingThatStandsAlone)
{
    overlay.select(BuildTool::Farm);

    pressAt(Cell{.x = 2, .y = 3}, MouseButton::Left);

    EXPECT_FALSE(drag.active());

    send(
        PointerButtonReleased{
            .button = MouseButton::Left,
            .position = pixelOf(Cell{.x = 8, .y = 9})});

    world.commit();

    EXPECT_EQ(paths.size(), 0U);
    EXPECT_EQ(
        (world.view<antwika::game::Building, Cell>().size()), 1U);
}

TEST_F(GridSinkTest, LeftRelease_FillsTheBoxADragOfHousesCrossed)
{
    overlay.select(BuildTool::House);

    pressAt(Cell{.x = 2, .y = 3}, MouseButton::Left);

    EXPECT_TRUE(drag.active());

    send(PointerMoved{.position = pixelOf(Cell{.x = 4, .y = 5})});
    send(
        PointerButtonReleased{
            .button = MouseButton::Left,
            .position = pixelOf(Cell{.x = 4, .y = 5})});

    world.commit();

    EXPECT_FALSE(drag.active());
    EXPECT_EQ(
        (world.view<antwika::game::Building, Cell>().size()), 9U);
    EXPECT_EQ(paths.size(), 0U);
}

TEST_F(GridSinkTest, LeftRelease_LaysOneHouseForADragThatNeverMoved)
{
    overlay.select(BuildTool::House);

    clickAt(Cell{.x = 2, .y = 3}, MouseButton::Left);
    send(
        PointerButtonReleased{
            .button = MouseButton::Left,
            .position = pixelOf(Cell{.x = 2, .y = 3})});

    world.commit();

    EXPECT_EQ(
        (world.view<antwika::game::Building, Cell>().size()), 1U);
}

TEST_F(GridSinkTest, LeftPress_CancelsTheDragBeforeIt)
{
    pressAt(Cell{.x = 2, .y = 3}, MouseButton::Left);
    send(PointerMoved{.position = pixelOf(Cell{.x = 8, .y = 3})});

    overlay.select(BuildTool::Farm);
    pressAt(Cell{.x = 2, .y = 8}, MouseButton::Left);

    EXPECT_FALSE(drag.active());

    send(
        PointerButtonReleased{
            .button = MouseButton::Left,
            .position = pixelOf(Cell{.x = 8, .y = 3})});

    EXPECT_EQ(paths.size(), 0U);
}

TEST_F(GridSinkTest, LeftRelease_LaysNothingWithoutADrag)
{
    send(
        PointerButtonReleased{
            .button = MouseButton::Left,
            .position = pixelOf(Cell{.x = 3, .y = 3})});

    EXPECT_EQ(paths.size(), 0U);
    EXPECT_FALSE(drag.active());
}

TEST_F(GridSinkTest, RightRelease_LeavesTheDragAlone)
{
    pressAt(Cell{.x = 2, .y = 3}, MouseButton::Left);
    send(
        PointerButtonReleased{
            .button = MouseButton::Right,
            .position = pixelOf(Cell{.x = 5, .y = 3})});

    EXPECT_TRUE(drag.active());
    EXPECT_EQ(paths.size(), 0U);
}

TEST_F(GridSinkTest, LeftRelease_EndsTheDragEvenOverTheToolbar)
{
    pressAt(Cell{.x = 2, .y = 3}, MouseButton::Left);

    overlay.set({}, {}, true);

    send(
        PointerButtonReleased{
            .button = MouseButton::Left,
            .position = pixelOf(Cell{.x = 4, .y = 3})});

    EXPECT_FALSE(drag.active());
    EXPECT_EQ(paths.size(), 3U);
}

TEST_F(GridSinkTest, PointerMoved_ExtendsNoDragThatIsNotUnderWay)
{
    send(PointerMoved{.position = pixelOf(Cell{.x = 9, .y = 9})});

    clickAt(Cell{.x = 2, .y = 3}, MouseButton::Left);
    send(
        PointerButtonReleased{
            .button = MouseButton::Left,
            .position = pixelOf(Cell{.x = 2, .y = 3})});

    EXPECT_EQ(paths.size(), 1U);
}

TEST_F(GridSinkTest, KeyPress_ChangesNothingOnTheGrid)
{
    const auto before = camera.pan();

    send(antwika::input::KeyPressed{.key = antwika::input::Key::A});

    EXPECT_EQ(paths.size(), 0U);
    EXPECT_EQ(before.x, camera.pan().x);
    EXPECT_FALSE(drag.active());
}

TEST_F(GridSinkTest, LeftClick_PaysForTheRoadTileItLays)
{
    clickAt(Cell{.x = 3, .y = 4}, MouseButton::Left);

    EXPECT_EQ(state.money, kStartingMoney - kRoadCost);
}

TEST_F(GridSinkTest, LeftClick_PaysNothingForARefusedTile)
{
    clickAt(Cell{.x = 3, .y = 4}, MouseButton::Left);
    clickAt(Cell{.x = 3, .y = 4}, MouseButton::Left);

    EXPECT_EQ(state.money, kStartingMoney - kRoadCost);
}

TEST_F(GridSinkTest, LeftClick_PaysForTheBuildingItPlaces)
{
    overlay.select(BuildTool::House);

    clickAt(Cell{.x = 3, .y = 4}, MouseButton::Left);

    EXPECT_EQ(
        state.money,
        kStartingMoney - costOf(antwika::game::BuildingKind::House));
}

TEST_F(GridSinkTest, LeftClick_PaysNothingForARefusedBuilding)
{
    overlay.select(BuildTool::House);

    clickAt(Cell{.x = 3, .y = 4}, MouseButton::Left);
    clickAt(Cell{.x = 3, .y = 4}, MouseButton::Left);

    EXPECT_EQ(
        state.money,
        kStartingMoney - costOf(antwika::game::BuildingKind::House));
}

TEST_F(GridSinkTest, LeftDrag_PaysForEveryTileTheRunLays)
{
    pressAt(Cell{.x = 2, .y = 3}, MouseButton::Left);
    send(PointerMoved{.position = pixelOf(Cell{.x = 5, .y = 3})});
    send(
        PointerButtonReleased{
            .button = MouseButton::Left,
            .position = pixelOf(Cell{.x = 5, .y = 3})});

    EXPECT_EQ(paths.size(), 4U);
    EXPECT_EQ(state.money, kStartingMoney - 4 * kRoadCost);
}

TEST_F(GridSinkTest, LeftDrag_PaysNothingForRoadAlreadyLaid)
{
    clickAt(Cell{.x = 3, .y = 3}, MouseButton::Left);

    pressAt(Cell{.x = 2, .y = 3}, MouseButton::Left);
    send(PointerMoved{.position = pixelOf(Cell{.x = 5, .y = 3})});
    send(
        PointerButtonReleased{
            .button = MouseButton::Left,
            .position = pixelOf(Cell{.x = 5, .y = 3})});

    EXPECT_EQ(paths.size(), 4U);
    EXPECT_EQ(state.money, kStartingMoney - 4 * kRoadCost);
}

TEST_F(GridSinkTest, RightPress_PlacesAWalkerForNothing)
{
    constexpr Cell target{.x = 2, .y = 2};
    clickAt(target, MouseButton::Left);

    clickAt(target, MouseButton::Right);
    clickAt(target, MouseButton::Right);

    EXPECT_EQ(walkerCount(), 1U);
    EXPECT_EQ(state.money, kStartingMoney - kRoadCost);
}

TEST_F(GridSinkTest, LeftClick_StillPlacesWithTheBankBelowZero)
{
    state.money = 0;

    clickAt(Cell{.x = 3, .y = 4}, MouseButton::Left);

    EXPECT_TRUE(paths.has(Cell{.x = 3, .y = 4}));
    EXPECT_EQ(state.money, -kRoadCost);
}

TEST_F(GridSinkTest, Raze_TearsDownTheBuildingUnderTheClick)
{
    overlay.select(BuildTool::House);
    clickAt(Cell{.x = 3, .y = 4}, MouseButton::Left);
    tick();

    overlay.select(BuildTool::Raze);
    clickAt(Cell{.x = 3, .y = 4}, MouseButton::Left);
    tick();

    EXPECT_EQ(built.size(), 0U);
    EXPECT_EQ(
        (world.view<antwika::game::Building, Cell>().size()), 0U);
}

TEST_F(GridSinkTest, Raze_TearsDownABlockClickedAwayFromItsOrigin)
{
    overlay.select(BuildTool::Farm);
    clickAt(Cell{.x = 3, .y = 4}, MouseButton::Left);
    tick();

    overlay.select(BuildTool::Raze);
    clickAt(Cell{.x = 4, .y = 5}, MouseButton::Left);
    tick();

    EXPECT_EQ(built.size(), 0U);
    EXPECT_EQ(
        (world.view<antwika::game::Building, Cell>().size()), 0U);
}

TEST_F(GridSinkTest, Raze_TakesUpTheRoadUnderTheClick)
{
    clickAt(Cell{.x = 3, .y = 4}, MouseButton::Left);
    clickAt(Cell{.x = 4, .y = 4}, MouseButton::Left);
    tick();

    overlay.select(BuildTool::Raze);
    clickAt(Cell{.x = 4, .y = 4}, MouseButton::Left);
    tick();

    EXPECT_FALSE(paths.has(Cell{.x = 4, .y = 4}));
    EXPECT_TRUE(paths.has(Cell{.x = 3, .y = 4}));
    EXPECT_EQ(paths.size(), 1U);
    EXPECT_EQ(pathEntityCount(), 1U);
}

TEST_F(GridSinkTest, Raze_DoesNothingOnBareGround)
{
    overlay.select(BuildTool::Raze);
    clickAt(Cell{.x = 3, .y = 4}, MouseButton::Left);
    tick();

    EXPECT_EQ(paths.size(), 0U);
    EXPECT_EQ(built.size(), 0U);
}

TEST_F(GridSinkTest, Raze_LeavesABlockPlacedInTheSameTickStanding)
{
    overlay.select(BuildTool::House);
    clickAt(Cell{.x = 3, .y = 4}, MouseButton::Left);

    overlay.select(BuildTool::Raze);
    clickAt(Cell{.x = 3, .y = 4}, MouseButton::Left);
    tick();

    EXPECT_EQ(built.size(), 1U);
    EXPECT_EQ(
        (world.view<antwika::game::Building, Cell>().size()), 1U);
}

TEST_F(GridSinkTest, Raze_ChargesPerBuildingRemoved)
{
    overlay.select(BuildTool::House);
    clickAt(Cell{.x = 3, .y = 4}, MouseButton::Left);
    tick();

    const auto before = state.money;

    overlay.select(BuildTool::Raze);
    clickAt(Cell{.x = 3, .y = 4}, MouseButton::Left);
    tick();

    EXPECT_EQ(state.money, before - antwika::game::kRazeCost);
}

TEST_F(GridSinkTest, Raze_ChargesPerRoadTileTakenUp)
{
    clickAt(Cell{.x = 3, .y = 4}, MouseButton::Left);
    tick();

    const auto before = state.money;

    overlay.select(BuildTool::Raze);
    clickAt(Cell{.x = 3, .y = 4}, MouseButton::Left);
    tick();

    EXPECT_EQ(state.money, before - antwika::game::kRazeCost);
}

TEST_F(GridSinkTest, Raze_ChargesNothingOnBareGround)
{
    const auto before = state.money;

    overlay.select(BuildTool::Raze);
    clickAt(Cell{.x = 9, .y = 9}, MouseButton::Left);
    tick();

    EXPECT_EQ(state.money, before);
}

TEST_F(GridSinkTest, RightPress_LeavesRazeModeWithoutDroppingAWalker)
{
    constexpr Cell target{.x = 2, .y = 2};
    clickAt(target, MouseButton::Left);
    tick();

    overlay.select(BuildTool::Raze);
    clickAt(target, MouseButton::Right);

    EXPECT_FALSE(overlay.tool().has_value());
    EXPECT_EQ(walkerCount(), 0U);
}

TEST_F(GridSinkTest, Raze_LeavesEveryOtherBuildingStanding)
{
    overlay.select(BuildTool::House);
    clickAt(Cell{.x = 1, .y = 1}, MouseButton::Left);
    tick();
    clickAt(Cell{.x = 5, .y = 5}, MouseButton::Left);
    tick();

    overlay.select(BuildTool::Raze);
    clickAt(Cell{.x = 5, .y = 5}, MouseButton::Left);
    tick();

    EXPECT_EQ(built.size(), 1U);
    EXPECT_TRUE(built.has(Cell{.x = 1, .y = 1}));
    EXPECT_EQ(
        (world.view<antwika::game::Building, Cell>().size()), 1U);
}

TEST_F(GridSinkTest, Raze_LeavesARoadLaidInTheSameTickDown)
{
    clickAt(Cell{.x = 3, .y = 4}, MouseButton::Left);

    overlay.select(BuildTool::Raze);
    clickAt(Cell{.x = 3, .y = 4}, MouseButton::Left);
    tick();

    EXPECT_TRUE(paths.has(Cell{.x = 3, .y = 4}));
    EXPECT_EQ(pathEntityCount(), 1U);
}

TEST_F(GridSinkTest, Raze_ClearsDebrisAndFreesItsBlock)
{
    const auto debris = world.create();
    world.add<Cell>(debris, Cell{.x = 3, .y = 4});
    world.add<antwika::game::Ruin>(
        debris,
        antwika::game::Ruin{
            .kind = antwika::game::BuildingKind::Farm,
            .state = antwika::game::RuinState::Debris,
            .ticksUntilOut = 0});
    world.commit();
    (void)built.insert(
        Cell{.x = 3, .y = 4},
        antwika::game::footprintOf(antwika::game::BuildingKind::Farm));

    const auto before = state.money;

    overlay.select(BuildTool::Raze);
    clickAt(Cell{.x = 4, .y = 5}, MouseButton::Left);
    tick();

    EXPECT_FALSE(world.alive(debris));
    EXPECT_FALSE(built.has(Cell{.x = 3, .y = 4}));
    EXPECT_FALSE(built.has(Cell{.x = 4, .y = 5}));
    EXPECT_EQ(state.money, before - antwika::game::kRazeCost);
}

TEST_F(GridSinkTest, Raze_ClearsAFireStillBurning)
{
    const auto fire = world.create();
    world.add<Cell>(fire, Cell{.x = 3, .y = 4});
    world.add<antwika::game::Ruin>(
        fire, antwika::game::Ruin{.kind = antwika::game::BuildingKind::House});
    world.commit();
    (void)built.insert(
        Cell{.x = 3, .y = 4},
        antwika::game::footprintOf(antwika::game::BuildingKind::House));

    overlay.select(BuildTool::Raze);
    clickAt(Cell{.x = 3, .y = 4}, MouseButton::Left);
    tick();

    EXPECT_FALSE(world.alive(fire));
    EXPECT_FALSE(built.has(Cell{.x = 3, .y = 4}));
}

TEST_F(GridSinkTest, Raze_LeavesARuinStandingElsewhereAlone)
{
    const auto debris = world.create();
    world.add<Cell>(debris, Cell{.x = 6, .y = 6});
    world.add<antwika::game::Ruin>(
        debris,
        antwika::game::Ruin{
            .kind = antwika::game::BuildingKind::House,
            .state = antwika::game::RuinState::Debris,
            .ticksUntilOut = 0});
    world.commit();
    (void)built.insert(
        Cell{.x = 6, .y = 6},
        antwika::game::footprintOf(antwika::game::BuildingKind::House));

    const auto standing = world.create();
    world.add<Cell>(standing, Cell{.x = 1, .y = 1});
    world.add<antwika::game::Ruin>(
        standing,
        antwika::game::Ruin{
            .kind = antwika::game::BuildingKind::House,
            .state = antwika::game::RuinState::Debris,
            .ticksUntilOut = 0});
    world.commit();
    (void)built.insert(
        Cell{.x = 1, .y = 1},
        antwika::game::footprintOf(antwika::game::BuildingKind::House));

    overlay.select(BuildTool::Raze);
    clickAt(Cell{.x = 1, .y = 1}, MouseButton::Left);
    tick();

    EXPECT_FALSE(world.alive(standing));
    EXPECT_TRUE(world.alive(debris));
    EXPECT_TRUE(built.has(Cell{.x = 6, .y = 6}));
}

TEST_F(GridSinkTest, Place_RefusesABuildingOnDebris)
{
    const auto debris = world.create();
    world.add<Cell>(debris, Cell{.x = 3, .y = 4});
    world.add<antwika::game::Ruin>(
        debris,
        antwika::game::Ruin{
            .kind = antwika::game::BuildingKind::House,
            .state = antwika::game::RuinState::Debris,
            .ticksUntilOut = 0});
    world.commit();
    (void)built.insert(
        Cell{.x = 3, .y = 4},
        antwika::game::footprintOf(antwika::game::BuildingKind::House));

    const auto before = state.money;

    overlay.select(BuildTool::House);
    clickAt(Cell{.x = 3, .y = 4}, MouseButton::Left);
    tick();

    EXPECT_EQ((world.view<antwika::game::Building, Cell>().size()), 0U);
    EXPECT_EQ(state.money, before);
}

TEST_F(GridSinkTest, LeftClick_PlacesAHouseAlreadyWatered)
{
    overlay.select(BuildTool::House);
    clickAt(Cell{.x = 2, .y = 3}, MouseButton::Left);
    world.commit();

    const auto houses =
        world.view<antwika::game::Building, Cell>();
    ASSERT_EQ(houses.size(), 1U);
    EXPECT_EQ(
        antwika::game::coverageOf(
            world, *houses.begin(), antwika::game::Service::Water),
        antwika::game::kCoverageFull);
}

TEST_F(GridSinkTest, LeftClick_PlacesAFarmWithNoCoverageAtAll)
{
    overlay.select(BuildTool::Farm);
    clickAt(Cell{.x = 2, .y = 3}, MouseButton::Left);
    world.commit();

    const auto farms =
        world.view<antwika::game::Building, Cell>();
    ASSERT_EQ(farms.size(), 1U);
    EXPECT_EQ(
        antwika::game::coverageOf(world, *farms.begin()),
        antwika::game::Coverage{});
}

TEST_F(GridSinkTest, RightPress_PutsNoWalkerOnBareGroundOnceCancelled)
{
    clickAt(Cell{.x = 2, .y = 2}, MouseButton::Right);
    clickAt(Cell{.x = 2, .y = 2}, MouseButton::Right);

    EXPECT_FALSE(overlay.tool().has_value());
    EXPECT_EQ(walkerCount(), 0U);
}

TEST_F(GridSinkTest, LeftClick_PaysTheConfiguredRoadCost)
{
    GameConfig config;
    config.roadCost = 9;
    GridSink tuned{
        world,
        paths,
        camera,
        kExtent,
        scheduler,
        input,
        overlay,
        cities,
        built,
        drag,
        state,
        config};

    for (const InputEvent &pointer :
         {InputEvent{PointerButtonPressed{
              .button = MouseButton::Left,
              .position = pixelOf(Cell{.x = 3, .y = 4})}},
          InputEvent{PointerButtonReleased{
              .button = MouseButton::Left,
              .position = pixelOf(Cell{.x = 3, .y = 4})}}})
    {
        const auto event =
            TickEvent{.tick = 0, .event = codec.encode(pointer)};

        input.handle(event);
        tuned.handle(event);
    }

    EXPECT_EQ(state.money, antwika::game::kStartingMoney - 9);
}

TEST_F(GridSinkTest, LeftPress_LaysNothingUntilTheButtonComesUp)
{
    constexpr Cell target{.x = 3, .y = 4};

    pressAt(target, MouseButton::Left);

    EXPECT_EQ(paths.size(), 0U);

    send(
        PointerButtonReleased{
            .button = MouseButton::Left,
            .position = pixelOf(target)});

    EXPECT_TRUE(paths.has(target));
}

TEST_F(GridSinkTest, LeftPress_PlacesNoBuildingUntilTheButtonComesUp)
{
    overlay.select(BuildTool::Farm);

    pressAt(Cell{.x = 3, .y = 4}, MouseButton::Left);
    world.commit();

    EXPECT_EQ(
        (world.view<antwika::game::Building, Cell>().size()), 0U);

    send(
        PointerButtonReleased{
            .button = MouseButton::Left,
            .position = pixelOf(Cell{.x = 3, .y = 4})});
    world.commit();

    EXPECT_EQ(
        (world.view<antwika::game::Building, Cell>().size()), 1U);
}

TEST_F(GridSinkTest, LeftRelease_PlacesWhereTheButtonCameUp)
{
    overlay.select(BuildTool::Farm);

    pressAt(Cell{.x = 3, .y = 4}, MouseButton::Left);
    send(
        PointerButtonReleased{
            .button = MouseButton::Left,
            .position = pixelOf(Cell{.x = 8, .y = 9})});

    world.commit();

    EXPECT_TRUE(built.has(Cell{.x = 8, .y = 9}));
    EXPECT_FALSE(built.has(Cell{.x = 3, .y = 4}));
}

TEST_F(GridSinkTest, LeftRelease_PlacesNothingWithNoPressBeforeIt)
{
    overlay.select(BuildTool::Farm);

    send(
        PointerButtonReleased{
            .button = MouseButton::Left,
            .position = pixelOf(Cell{.x = 3, .y = 4})});

    world.commit();

    EXPECT_EQ(
        (world.view<antwika::game::Building, Cell>().size()), 0U);
}

TEST_F(GridSinkTest, LeftRelease_PlacesNothingOnceTheToolIsPutDown)
{
    overlay.select(BuildTool::Farm);

    pressAt(Cell{.x = 3, .y = 4}, MouseButton::Left);

    overlay.clearTool();

    send(
        PointerButtonReleased{
            .button = MouseButton::Left,
            .position = pixelOf(Cell{.x = 3, .y = 4})});

    world.commit();

    EXPECT_EQ(
        (world.view<antwika::game::Building, Cell>().size()), 0U);
}
