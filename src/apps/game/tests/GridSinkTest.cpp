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

#include "antwika/game/BuildTool.hpp"
#include "antwika/game/Building.hpp"
#include "antwika/game/BuildingIndex.hpp"
#include "antwika/game/Cost.hpp"
#include "antwika/game/Ruin.hpp"
#include "antwika/game/GameState.hpp"
#include "antwika/game/LiveGrid.hpp"
#include "antwika/game/Camera.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/GridSink.hpp"
#include "antwika/game/WorldMapState.hpp"
#include "antwika/game/InputFold.hpp"
#include "antwika/game/IsoProjection.hpp"
#include "antwika/game/Path.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/RoadDrag.hpp"
#include "antwika/game/UiOverlay.hpp"
#include "antwika/game/Walker.hpp"

using antwika::ecs::SystemScheduler;
using antwika::ecs::World;
using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::game::BuildingIndex;
using antwika::game::BuildTool;
using antwika::game::LiveGrid;
using antwika::game::Camera;
using antwika::game::Cell;
using antwika::game::cellCentre;
using antwika::game::costOf;
using antwika::game::GameState;
using antwika::game::GridExtent;
using antwika::game::GridSink;
using antwika::game::kRoadCost;
using antwika::game::kStartingMoney;
using antwika::game::WorldMap;
using antwika::game::WorldMapState;
using antwika::game::InputFold;
using antwika::game::Path;
using antwika::game::PathIndex;
using antwika::game::RoadDrag;
using antwika::game::UiOverlay;
using antwika::game::Walker;
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
        // Where on screen a cell's middle is, as a click would report it.
        [[nodiscard]] Position pixelOf(Cell cell) const
        {
            const auto point = cellCentre(cell, camera);
            return Position{.x = point.x, .y = point.y};
        }

        // Through the fold first, as bootstrap() registers it.
        // What the sink reads is what the fold was just given.
        void dispatch(const TickEvent &event)
        {
            input.handle(event);
            sink.handle(event);
        }

        void send(const InputEvent &event)
        {
            dispatch(TickEvent{.tick = 0, .event = codec.encode(event)});
        }

        void clickAt(Cell cell, MouseButton button)
        {
            send(
                PointerButtonPressed{
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

        // Nothing has drawn a toolbar, so nothing is covered.
        // The tests that care say otherwise for themselves.
        UiOverlay overlay;

        // One city, permanently open, as a run with no world map has.
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
            state};
    };
} // namespace

TEST_F(GridSinkTest, LeftPress_LaysAPathAtTheClickedCell)
{
    constexpr Cell target{.x = 3, .y = 4};

    clickAt(target, MouseButton::Left);

    EXPECT_TRUE(paths.has(target));
    EXPECT_EQ(paths.size(), 1U);
    EXPECT_EQ(pathEntityCount(), 1U);
}

TEST_F(GridSinkTest, LeftPress_LaysNoSecondPathOnTheSameCell)
{
    constexpr Cell target{.x = 3, .y = 4};

    clickAt(target, MouseButton::Left);
    clickAt(target, MouseButton::Left);

    EXPECT_EQ(paths.size(), 1U);
    EXPECT_EQ(pathEntityCount(), 1U);
}

TEST_F(GridSinkTest, LeftPress_LaysNothingOutsideTheExtent)
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

    EXPECT_EQ(walkerCount(), 1U);
}

TEST_F(GridSinkTest, RightPress_PutsNoWalkerOnBareGround)
{
    clickAt(Cell{.x = 2, .y = 2}, MouseButton::Right);

    EXPECT_EQ(walkerCount(), 0U);
}

// One button, two meanings, and the palette decides which.
// A building tool selected makes a right press a cancel.
TEST_F(GridSinkTest, RightPress_LeavesBuildModeWithABuildingToolSelected)
{
    overlay.select(BuildTool::House);

    clickAt(Cell{.x = 2, .y = 2}, MouseButton::Right);

    EXPECT_FALSE(overlay.tool().has_value());
}

// Cancelling reaches a state of its own rather than a fallback.
// So the press after one lays no road nobody asked for.
TEST_F(GridSinkTest, LeftPress_LaysNothingOnceThePaletteIsPutDown)
{
    constexpr Cell target{.x = 3, .y = 4};

    overlay.select(BuildTool::House);
    clickAt(target, MouseButton::Right);
    clickAt(target, MouseButton::Left);

    EXPECT_EQ(paths.size(), 0U);
    EXPECT_EQ(pathEntityCount(), 0U);
    EXPECT_EQ(built.size(), 0U);
}

// And the cancel is the whole of what that press does.
// A walker as well would be one press doing both meanings.
TEST_F(GridSinkTest, RightPress_PlacesNoWalkerWhileLeavingBuildMode)
{
    constexpr Cell target{.x = 2, .y = 2};
    clickAt(target, MouseButton::Left);

    overlay.select(BuildTool::Farm);
    clickAt(target, MouseButton::Right);

    EXPECT_EQ(walkerCount(), 0U);

    // The next one drops one, nothing being selected any more.
    clickAt(target, MouseButton::Right);

    EXPECT_EQ(walkerCount(), 1U);
}

// Cancelling twice is cancelling once.
// Nothing selected is where a cancel leaves it, so it cancels nothing.
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

// The road is a tool a cancel does not reach past.
// It places something, so a right press with it up is a walker.
TEST_F(GridSinkTest, RightPress_IsAWalkerWithTheRoadToolSelected)
{
    constexpr Cell target{.x = 2, .y = 2};
    clickAt(target, MouseButton::Left);

    clickAt(target, MouseButton::Right);

    EXPECT_EQ(overlay.tool(), BuildTool::Road);
    EXPECT_EQ(walkerCount(), 1U);
}

// What the toolbar covers, it covers from the grid too.
// A right press on the bar is not a press on the world behind it.
TEST_F(GridSinkTest, RightPress_LeavesBuildModeAloneWhereTheToolbarIs)
{
    overlay.select(BuildTool::House);
    overlay.set({}, true);

    clickAt(Cell{.x = 2, .y = 2}, MouseButton::Right);

    EXPECT_EQ(overlay.tool(), BuildTool::House);
}

// Leaving build mode is about the palette, not about a cell.
// So it happens wherever on the grid the press landed.
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

// Placement is on the press, so a release must not count as a second one.
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

// The ordering guarantee, asserted rather than assumed.
// A tick's input arrives before its engine.tick.
// So a click resolves against the camera the same tick's scroll left.
TEST_F(GridSinkTest, Scroll_ThenAClickResolvesAgainstTheNewZoom)
{
    constexpr Cell target{.x = 5, .y = 6};

    send(PointerMoved{.position = pixelOf(target)});
    send(PointerScrolled{.vertical = 1});

    // Recomputed against the zoomed camera, which is the point.
    clickAt(target, MouseButton::Left);

    EXPECT_TRUE(paths.has(target));
}

TEST_F(GridSinkTest, Tick_RunsTheSchedulerAndCommitsTheWorld)
{
    constexpr Cell target{.x = 3, .y = 3};
    clickAt(target, MouseButton::Left);

    // Staged by the click, and only visible after the tick's commit.
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

    // The button is still held, so this still pans.
    // Held state survives a tick even though the edges do not.
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

// What the toolbar covers, it covers from the grid too.
TEST_F(GridSinkTest, LeftPress_LaysNoPathWhereTheToolbarIs)
{
    constexpr Cell target{.x = 3, .y = 4};

    overlay.set({}, true);
    clickAt(target, MouseButton::Left);

    EXPECT_FALSE(paths.has(target));
    EXPECT_EQ(paths.size(), 0U);
}

TEST_F(GridSinkTest, Scroll_DoesNotZoomWhereTheToolbarIs)
{
    const auto before = camera;

    overlay.set({}, true);
    send(PointerScrolled{.vertical = 1});

    EXPECT_EQ(before, camera);
}

// A pan begun on the grid must not stop dead under the bar.
TEST_F(GridSinkTest, Move_KeepsPanningAcrossTheToolbar)
{
    send(
        PointerButtonPressed{
            .button = MouseButton::Middle,
            .position = Position{.x = 100, .y = 100}});

    const auto before = camera.pan();

    overlay.set({}, true);
    send(PointerMoved{.position = Position{.x = 110, .y = 100}});

    EXPECT_EQ(before.x + 10, camera.pan().x);
}

// A press on the world map is not a press on any city's grid.
// The mode gate says so a tick later; this says so at once.
TEST_F(GridSinkTest, LeftPress_LaysNothingWhileNoCityIsOpen)
{
    cities.closeCity(live);

    clickAt(Cell{.x = 3, .y = 4}, MouseButton::Left);

    EXPECT_FALSE(paths.has(Cell{.x = 3, .y = 4}));
    EXPECT_EQ(0U, pathEntityCount());
}

// And the tick still arrives, so the world still commits.
TEST_F(GridSinkTest, Tick_StillRunsWhileNoCityIsOpen)
{
    clickAt(Cell{.x = 3, .y = 4}, MouseButton::Left);
    cities.closeCity(live);

    tick();

    EXPECT_EQ(1U, pathEntityCount());
}

// A left press marks where a run of road starts.
// The release lays every cell of the route between the two.
TEST_F(GridSinkTest, LeftDrag_LaysTheWholeRunOnTheRelease)
{
    clickAt(Cell{.x = 2, .y = 3}, MouseButton::Left);
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

// A route that does not exist is not half-built -- see RoadPlan.
TEST_F(GridSinkTest, LeftDrag_LaysNothingWhenNoRouteExists)
{
    // A wall of buildings across the grid, with the goal beyond it.
    for (std::int32_t y = 0; y < kExtent.height; ++y)
    {
        built.insert(Cell{.x = 4, .y = y}, antwika::game::Footprint{});
    }

    clickAt(Cell{.x = 2, .y = 3}, MouseButton::Left);
    send(
        PointerButtonReleased{
            .button = MouseButton::Left,
            .position = pixelOf(Cell{.x = 6, .y = 3})});

    // Only the pressed cell, which was laid by the press itself.
    EXPECT_EQ(paths.size(), 1U);
    EXPECT_TRUE(paths.has(Cell{.x = 2, .y = 3}));
}

// A drag runs from the press to the release and holds nothing still.
// It used to pause the run for the length of one -- see PauseState.
TEST_F(GridSinkTest, LeftDrag_IsUnderWayFromThePressToTheRelease)
{
    clickAt(Cell{.x = 2, .y = 3}, MouseButton::Left);

    EXPECT_TRUE(drag.active());

    send(
        PointerButtonReleased{
            .button = MouseButton::Left,
            .position = pixelOf(Cell{.x = 3, .y = 3})});

    EXPECT_FALSE(drag.active());
}

// A building is placed on the press alone, and starts no drag.
TEST_F(GridSinkTest, LeftPress_StartsNoDragForABuildingTool)
{
    overlay.select(BuildTool::House);

    clickAt(Cell{.x = 2, .y = 3}, MouseButton::Left);

    EXPECT_FALSE(drag.active());

    send(
        PointerButtonReleased{
            .button = MouseButton::Left,
            .position = pixelOf(Cell{.x = 6, .y = 3})});

    EXPECT_EQ(paths.size(), 0U);
}

// A fresh press ends the gesture before it, laying none of its route.
TEST_F(GridSinkTest, LeftPress_CancelsTheDragBeforeIt)
{
    clickAt(Cell{.x = 2, .y = 3}, MouseButton::Left);
    send(PointerMoved{.position = pixelOf(Cell{.x = 8, .y = 3})});

    // A building tool, so the second press starts no drag of its own.
    overlay.select(BuildTool::House);
    clickAt(Cell{.x = 2, .y = 8}, MouseButton::Left);

    EXPECT_FALSE(drag.active());

    send(
        PointerButtonReleased{
            .button = MouseButton::Left,
            .position = pixelOf(Cell{.x = 8, .y = 3})});

    // The first press laid its own cell, and the cancel laid nothing.
    EXPECT_EQ(paths.size(), 1U);
}

// A release with no drag behind it is an ordinary no-op.
TEST_F(GridSinkTest, LeftRelease_LaysNothingWithoutADrag)
{
    send(
        PointerButtonReleased{
            .button = MouseButton::Left,
            .position = pixelOf(Cell{.x = 3, .y = 3})});

    EXPECT_EQ(paths.size(), 0U);
    EXPECT_FALSE(drag.active());
}

// Only the left button ends a road drag.
TEST_F(GridSinkTest, RightRelease_LeavesTheDragAlone)
{
    clickAt(Cell{.x = 2, .y = 3}, MouseButton::Left);
    send(
        PointerButtonReleased{
            .button = MouseButton::Right,
            .position = pixelOf(Cell{.x = 5, .y = 3})});

    EXPECT_TRUE(drag.active());
    EXPECT_EQ(paths.size(), 1U);
}

// A gesture begun on the grid has to be able to end anywhere.
// Otherwise a drag let go over the bar would lay no road at all.
TEST_F(GridSinkTest, LeftRelease_EndsTheDragEvenOverTheToolbar)
{
    clickAt(Cell{.x = 2, .y = 3}, MouseButton::Left);

    overlay.set({}, true);

    send(
        PointerButtonReleased{
            .button = MouseButton::Left,
            .position = pixelOf(Cell{.x = 4, .y = 3})});

    EXPECT_FALSE(drag.active());
    EXPECT_EQ(paths.size(), 3U);
}

// A movement with no button behind it leaves no end cell behind.
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

// The grid takes the pointer and nothing else.
// A key is somebody else's business, and reaches no placement here.
TEST_F(GridSinkTest, KeyPress_ChangesNothingOnTheGrid)
{
    const auto before = camera.pan();

    send(antwika::input::KeyPressed{.key = antwika::input::Key::A});

    EXPECT_EQ(paths.size(), 0U);
    EXPECT_EQ(before.x, camera.pan().x);
    EXPECT_FALSE(drag.active());
}

// A placement is paid for out of the bank -- see GameState::money.
TEST_F(GridSinkTest, LeftPress_PaysForTheRoadTileItLays)
{
    clickAt(Cell{.x = 3, .y = 4}, MouseButton::Left);

    EXPECT_EQ(state.money, kStartingMoney - kRoadCost);
}

// A refused placement went up as nothing, so it costs nothing.
TEST_F(GridSinkTest, LeftPress_PaysNothingForARefusedTile)
{
    clickAt(Cell{.x = 3, .y = 4}, MouseButton::Left);
    clickAt(Cell{.x = 3, .y = 4}, MouseButton::Left);

    EXPECT_EQ(state.money, kStartingMoney - kRoadCost);
}

TEST_F(GridSinkTest, LeftPress_PaysForTheBuildingItPlaces)
{
    overlay.select(BuildTool::House);

    clickAt(Cell{.x = 3, .y = 4}, MouseButton::Left);

    EXPECT_EQ(
        state.money,
        kStartingMoney - costOf(antwika::game::BuildingKind::House));
}

TEST_F(GridSinkTest, LeftPress_PaysNothingForARefusedBuilding)
{
    overlay.select(BuildTool::House);

    clickAt(Cell{.x = 3, .y = 4}, MouseButton::Left);
    clickAt(Cell{.x = 3, .y = 4}, MouseButton::Left);

    EXPECT_EQ(
        state.money,
        kStartingMoney - costOf(antwika::game::BuildingKind::House));
}

// A dragged run's price is the road's times the tiles it put down.
TEST_F(GridSinkTest, LeftDrag_PaysForEveryTileTheRunLays)
{
    clickAt(Cell{.x = 2, .y = 3}, MouseButton::Left);
    send(PointerMoved{.position = pixelOf(Cell{.x = 5, .y = 3})});
    send(
        PointerButtonReleased{
            .button = MouseButton::Left,
            .position = pixelOf(Cell{.x = 5, .y = 3})});

    EXPECT_EQ(paths.size(), 4U);
    EXPECT_EQ(state.money, kStartingMoney - 4 * kRoadCost);
}

// A route through road already laid pays only for what it lays.
// The run below names four cells and one of them is already paved.
TEST_F(GridSinkTest, LeftDrag_PaysNothingForRoadAlreadyLaid)
{
    clickAt(Cell{.x = 3, .y = 3}, MouseButton::Left);

    clickAt(Cell{.x = 2, .y = 3}, MouseButton::Left);
    send(PointerMoved{.position = pixelOf(Cell{.x = 5, .y = 3})});
    send(
        PointerButtonReleased{
            .button = MouseButton::Left,
            .position = pixelOf(Cell{.x = 5, .y = 3})});

    EXPECT_EQ(paths.size(), 4U);
    EXPECT_EQ(state.money, kStartingMoney - 4 * kRoadCost);
}

// Only a placement spends; a walker is not one.
TEST_F(GridSinkTest, RightPress_PlacesAWalkerForNothing)
{
    constexpr Cell target{.x = 2, .y = 2};
    clickAt(target, MouseButton::Left);

    clickAt(target, MouseButton::Right);

    EXPECT_EQ(walkerCount(), 1U);
    EXPECT_EQ(state.money, kStartingMoney - kRoadCost);
}

// Nothing pays money in yet, so spending is never refused.
// A bank refused at zero would end a session for good.
// See GameState::money.
TEST_F(GridSinkTest, LeftPress_StillPlacesWithTheBankBelowZero)
{
    state.money = 0;

    clickAt(Cell{.x = 3, .y = 4}, MouseButton::Left);

    EXPECT_TRUE(paths.has(Cell{.x = 3, .y = 4}));
    EXPECT_EQ(state.money, -kRoadCost);
}

// The raze tool tears down instead of placing -- see GridSink.hpp.
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

// Any cell of a block razes the whole building.
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

// The razed cell is deliberately the second one laid.
// So the lookup loop walks past the first road to find it.
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

// The World hands out the last commit.
// A block placed this tick is in the index and not yet in the World.
// So it cannot be found to tear down until the next one.
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

// Tearing down costs too, per thing removed -- see Cost.hpp.
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

// A miss is charged as nothing, as a refused placement is.
TEST_F(GridSinkTest, Raze_ChargesNothingOnBareGround)
{
    const auto before = state.money;

    overlay.select(BuildTool::Raze);
    clickAt(Cell{.x = 9, .y = 9}, MouseButton::Left);
    tick();

    EXPECT_EQ(state.money, before);
}

// A destructive mode is put down exactly as a building tool is.
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

// The loop walks past a building the click is not on.
// The one standing elsewhere keeps standing.
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

// The same-tick rule a block already has, said for a road.
// The index alone erased would orphan the entity the commit lands.
TEST_F(GridSinkTest, Raze_LeavesARoadLaidInTheSameTickDown)
{
    clickAt(Cell{.x = 3, .y = 4}, MouseButton::Left);

    overlay.select(BuildTool::Raze);
    clickAt(Cell{.x = 3, .y = 4}, MouseButton::Left);
    tick();

    EXPECT_TRUE(paths.has(Cell{.x = 3, .y = 4}));
    EXPECT_EQ(pathEntityCount(), 1U);
}

// Debris holds its block, and the raze tool is what frees it.
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

    // Any cell of the block, exactly as a building is razed.
    overlay.select(BuildTool::Raze);
    clickAt(Cell{.x = 4, .y = 5}, MouseButton::Left);
    tick();

    EXPECT_FALSE(world.alive(debris));
    EXPECT_FALSE(built.has(Cell{.x = 3, .y = 4}));
    EXPECT_FALSE(built.has(Cell{.x = 4, .y = 5}));
    EXPECT_EQ(state.money, before - antwika::game::kRazeCost);
}

// A burning ruin may be razed too: the fire ends with the ground.
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

// The loop walks past a ruin the click is not on.
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

    // The click lands on the later ruin.
    // So the loop walks past the first one without touching it.
    overlay.select(BuildTool::Raze);
    clickAt(Cell{.x = 1, .y = 1}, MouseButton::Left);
    tick();

    EXPECT_FALSE(world.alive(standing));
    EXPECT_TRUE(world.alive(debris));
    EXPECT_TRUE(built.has(Cell{.x = 6, .y = 6}));
}

// Nothing may be built on debris; the ghost's canPlace() refuses it.
// The index is what refuses, so the sink refuses with it.
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
