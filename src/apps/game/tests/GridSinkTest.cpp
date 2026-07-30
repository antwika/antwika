#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/ecs/SystemScheduler.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/input/InputError.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/game/Camera.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/GridSink.hpp"
#include "antwika/game/IsoProjection.hpp"
#include "antwika/game/Path.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/UiOverlay.hpp"
#include "antwika/game/Walker.hpp"

using antwika::ecs::SystemScheduler;
using antwika::ecs::World;
using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::game::Camera;
using antwika::game::Cell;
using antwika::game::cellCentre;
using antwika::game::GridExtent;
using antwika::game::GridSink;
using antwika::game::Path;
using antwika::game::PathIndex;
using antwika::game::UiOverlay;
using antwika::game::Walker;
using antwika::input::InputError;
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

        void send(const InputEvent &event)
        {
            sink.handle(
                TickEvent{.tick = 0, .event = codec.encode(event)});
        }

        void clickAt(Cell cell, MouseButton button)
        {
            send(
                PointerButtonPressed{
                    .button = button, .position = pixelOf(cell)});
        }

        void tick()
        {
            sink.handle(
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
        Camera camera{antwika::gfx::Point{.x = 400, .y = 40}};
        SystemScheduler scheduler;
        InputEventCodec codec;

        // Nothing has drawn a toolbar, so nothing is covered.
        // The tests that care say otherwise for themselves.
        UiOverlay overlay;
        GridSink sink{
            world, paths, camera, kExtent, scheduler, codec, overlay};
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
    sink.handle(
        TickEvent{
            .tick = 0,
            .event = Event{.name = "game.score_increment"}});

    EXPECT_EQ(paths.size(), 0U);
    EXPECT_EQ(walkerCount(), 0U);
}

TEST_F(GridSinkTest, Handle_LetsAMalformedInputPayloadThrough)
{
    // The wire format is the codec's to police.
    // Its error surfaces rather than a second type saying the same.
    EXPECT_THROW(
        sink.handle(
            TickEvent{
                .tick = 0,
                .event = Event{
                    .name = "input.pointer_down",
                    .payload = "not json"}}),
        InputError);
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
