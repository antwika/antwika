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
#include "antwika/game/BuildingIndex.hpp"
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
using antwika::game::GridExtent;
using antwika::game::GridSink;
using antwika::game::WorldMap;
using antwika::game::WorldMapState;
using antwika::game::InputFold;
using antwika::game::Path;
using antwika::game::PathIndex;
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
        GridSink sink{
            world,
            paths,
            camera,
            kExtent,
            scheduler,
            input,
            overlay,
            cities,
            built};
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

    overlay.select(BuildTool::FoodSource);
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

    overlay.select(BuildTool::ArchitectPost);
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
