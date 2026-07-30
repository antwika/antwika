#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <sstream>

#include <vector>

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/mocks/MockEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/event/TickEventRecorder.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/replay/EngineLoopError.hpp>
#include <antwika/replay/ReplaySource.hpp>

#include "antwika/game/Camera.hpp"
#include "antwika/game/Events.hpp"
#include "antwika/game/Game.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Direction.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/IsoProjection.hpp"
#include "antwika/game/MainMenu.hpp"
#include "antwika/game/MenuSink.hpp"
#include "antwika/game/MenuState.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/Toolbar.hpp"
#include "antwika/game/UiCanvas.hpp"
#include "antwika/game/UiOverlay.hpp"

using antwika::event::Event;
using antwika::event::mocks::MockEventSink;
using antwika::event::TickEvent;
using antwika::event::TickEventRecorder;
using antwika::game::Camera;
using antwika::gfx::Point;
using antwika::game::GameState;
using antwika::game::GridExtent;
using antwika::game::PathIndex;
using antwika::input::InputEventCodec;
using antwika::log::mocks::MockLogger;
using antwika::replay::EngineLoopError;
using antwika::replay::ReplaySource;
using ::testing::NiceMock;

namespace
{
    constexpr GridExtent kExtent{.width = 16, .height = 16};

    // Bootstrap wires together already-unit-tested collaborators.
    // Re-testing their call sequences here would be redundant.
    // It would also be brittle to maintain over time.
    // These tests check the wiring end to end, black-box style.
    struct Harness
    {
        NiceMock<MockLogger> logger;
        NiceMock<MockEventSink> eventSink;
        InputEventCodec codec;
        Camera camera;
        PathIndex paths;

        antwika::game::GameSummary run(
            ReplaySource &source,
            antwika::time::Tick maxTicks,
            antwika::event::ITickEventSink *recorder = nullptr)
        {
            antwika::game::GameConfig config{
                .logger = logger,
                .eventSink = eventSink,
                .inputSource = source,
                .codec = codec,
                .extent = kExtent,
                .camera = camera,
                .paths = paths,
                .maxTicks = maxTicks};
            if (recorder != nullptr)
            {
                config.replayRecorder = *recorder;
            }

            return antwika::game::bootstrap(config);
        }
    };
} // namespace

TEST(BootstrapTest, Bootstrap_RunsScriptedTicksAndReturnsTheGameState)
{
    Harness harness;
    ReplaySource source({
        TickEvent{
            .tick = 1,
            .event = Event{
                .name = antwika::game::events::kScoreIncrement,
                .payload = R"({"amount":5})",
            },
        },
        TickEvent{
            .tick = 3,
            .event = Event{
                .name = antwika::game::events::kScoreIncrement,
                .payload = R"({"amount":2})",
            },
        },
        TickEvent{
            .tick = 4,
            .event = Event{.name = antwika::engine::events::kStop},
        },
    });

    const auto summary = harness.run(source, 10);

    EXPECT_EQ(
        summary.state, (GameState{.ticksProcessed = 5, .score = 7}));
}

TEST(BootstrapTest, Bootstrap_WithNoScriptedInputOnlyAdvancesTicks)
{
    Harness harness;
    ReplaySource source({
        TickEvent{
            .tick = 2,
            .event = Event{.name = antwika::engine::events::kStop},
        },
    });

    const auto summary = harness.run(source, 10);

    EXPECT_EQ(
        summary.state, (GameState{.ticksProcessed = 3, .score = 0}));
    EXPECT_TRUE(summary.paths.empty());
    EXPECT_TRUE(summary.walkers.empty());
}

// A caller wanting to persist a `--record` file has no pre-known script.
// It instead passes an optional replayRecorder.
// bootstrap() must register it so it observes every dispatched event.
TEST(BootstrapTest, Bootstrap_ForwardsDispatchedEventsToATickEventRecorder)
{
    Harness harness;
    ReplaySource source({
        TickEvent{
            .tick = 0,
            .event = Event{
                .name = antwika::game::events::kScoreIncrement,
                .payload = R"({"amount":5})",
            },
        },
        TickEvent{
            .tick = 0,
            .event = Event{.name = antwika::engine::events::kStop},
        },
    });
    TickEventRecorder recorder;

    harness.run(source, 10, &recorder);

    EXPECT_EQ(
        recorder.getEvents(),
        (std::vector<TickEvent>{
            TickEvent{
                .tick = 0,
                .event = Event{
                    .name = antwika::game::events::kScoreIncrement,
                    .payload = R"({"amount":5})",
                },
            },
            TickEvent{
                .tick = 0,
                .event = Event{.name = antwika::engine::events::kStop},
            },
            TickEvent{
                .tick = 0,
                .event = Event{.name = antwika::engine::events::kTick},
            },
        }));
}

// Safety valve: a run that never dispatches engine.stop must fail loudly.
// It should not hang or silently truncate once maxTicks is reached.
TEST(BootstrapTest, Bootstrap_ThrowsWhenMaxTicksIsReachedWithoutAStopEvent)
{
    Harness harness;
    ReplaySource source({});

    EXPECT_THROW(harness.run(source, 3), EngineLoopError);
}

// The whole feature, through the front door.
// Clicks lay a path, a click drops a walker, and the walker moves.
TEST(BootstrapTest, Bootstrap_LaysAPathDropsAWalkerAndWalksIt)
{
    Harness harness;
    const InputEventCodec codec;

    const auto pressAt = [&codec](antwika::game::Cell cell,
                                  antwika::input::MouseButton button)
    {
        const auto point =
            antwika::game::cellCentre(cell, antwika::game::Camera());
        return codec.encode(
            antwika::input::PointerButtonPressed{
                .button = button,
                .position = {.x = point.x, .y = point.y}});
    };

    const std::vector<TickEvent> script{
        TickEvent{
            .tick = 0,
            .event = pressAt(
                antwika::game::Cell{.x = 1, .y = 1},
                antwika::input::MouseButton::Left)},
        TickEvent{
            .tick = 0,
            .event = pressAt(
                antwika::game::Cell{.x = 2, .y = 1},
                antwika::input::MouseButton::Left)},
        TickEvent{
            .tick = 0,
            .event = pressAt(
                antwika::game::Cell{.x = 1, .y = 1},
                antwika::input::MouseButton::Right)},
        // Stopping on the same tick keeps this to exactly one step.
        // The two-cell path is a dead end at both ends.
        // A longer run would oscillate and say nothing about the move.
        TickEvent{
            .tick = 0,
            .event = Event{.name = antwika::engine::events::kStop}},
    };
    ReplaySource source(script);

    const auto summary = harness.run(source, 10);

    EXPECT_EQ(summary.paths.size(), 2U);
    ASSERT_EQ(summary.walkers.size(), 1U);

    // Dropped on (1,1) facing east, with (2,1) the only way on.
    EXPECT_EQ(summary.walkers[0].at, (antwika::game::Cell{.x = 2, .y = 1}));
    EXPECT_EQ(summary.walkers[0].facing, antwika::game::Direction::East);
}

namespace
{
    // Counts the ticks it observes, and touches nothing.
    class CountingObserver final : public antwika::ecs::ISystem
    {
    public:
        void update(antwika::ecs::World &, antwika::time::Tick) override
        {
            ++ticks;
        }

        std::size_t ticks = 0;
    };
} // namespace

// Observers run in a phase of their own, after the walk.
// A renderer registered here sees the generation this tick produced.
TEST(BootstrapTest, Bootstrap_RunsEveryObserverOncePerTick)
{
    Harness harness;
    ReplaySource source({
        TickEvent{
            .tick = 2,
            .event = Event{.name = antwika::engine::events::kStop},
        },
    });

    CountingObserver first;
    CountingObserver second;

    antwika::game::bootstrap(
        antwika::game::GameConfig{
            .logger = harness.logger,
            .eventSink = harness.eventSink,
            .inputSource = source,
            .codec = harness.codec,
            .extent = kExtent,
            .camera = harness.camera,
            .paths = harness.paths,
            .observers = {first, second},
            .maxTicks = 10});

    // Ticks 0, 1 and 2 all run, and the stop ends it after the third.
    EXPECT_EQ(first.ticks, 3U);
    EXPECT_EQ(second.ticks, 3U);
}

TEST(PrintSummaryTest, WritesTheStateTheCountsAndTheCamera)
{
    std::ostringstream out;
    const antwika::game::GameSummary summary{
        .state = {.ticksProcessed = 4, .score = 7},
        .paths = {{.x = 1, .y = 1}, {.x = 1, .y = 2}},
        .walkers = {},
        .buildings = {},
        .camera = Camera(Point{.x = 512, .y = 48})};

    antwika::game::printSummary(out, summary);

    EXPECT_EQ(
        out.str(),
        "Final state: ticksProcessed=4 score=7\n"
        "Paths laid: 2\n"
        "Walkers: 0\n"
        "Buildings: 0\n"
        "Camera: pan (512, 48) zoom 3\n");
}

TEST(PrintSummaryTest, WritesEveryWalkerWhereItStandsAndWhereItFaces)
{
    std::ostringstream out;
    const antwika::game::GameSummary summary{
        .state = {},
        .paths = {},
        .walkers =
            {{.at = {.x = 3, .y = 4},
              .facing = antwika::game::Direction::South}},
        .buildings = {},
        .camera = Camera(Point{.x = 0, .y = 0})};

    antwika::game::printSummary(out, summary);

    EXPECT_NE(
        out.str().find("  at (3, 4) facing 2\n"), std::string::npos);
}

TEST(PrintSummaryTest, WritesEveryBuildingWhereItStandsAndWhatItHolds)
{
    std::ostringstream out;
    const antwika::game::GameSummary summary{
        .state = {},
        .paths = {},
        .walkers = {},
        .buildings =
            {{.at = {.x = 6, .y = 2},
              .kind = antwika::game::BuildingKind::WaterSource,
              .held = 40,
              .capacity = 100}},
        .camera = Camera(Point{.x = 0, .y = 0})};

    antwika::game::printSummary(out, summary);

    EXPECT_NE(
        out.str().find("  at (6, 2) kind 2 stock 40/100\n"),
        std::string::npos);
}

namespace
{
    // Where an entry ends up is the layout's business.
    // So a test looks for a pixel that hits the one it means.
    // The same scan ReplayDeterminismTest does over the toolbar.
    [[nodiscard]] antwika::input::Position menuPixelOn(
        antwika::ui::WidgetId id, const antwika::game::MenuState &state)
    {
        const antwika::game::MainMenu menu;
        const auto canvas = antwika::game::kUiCanvas;

        for (std::int32_t y = 0;
             y < static_cast<std::int32_t>(canvas.height);
             y += 4)
        {
            for (std::int32_t x = 0;
                 x < static_cast<std::int32_t>(canvas.width);
                 x += 4)
            {
                const antwika::ui::Pointer pointer{
                    .position = Point{.x = x, .y = y}};

                if (menu.describe(canvas, pointer, state)
                        .interactions.hovered
                    == id)
                {
                    return antwika::input::Position{.x = x, .y = y};
                }
            }
        }

        return antwika::input::Position{.x = -1, .y = -1};
    }

    // Everything a menu run is wired out of, kept together.
    // A plain Harness has no overlay, so it has no canvas either.
    struct MenuHarness
    {
        NiceMock<MockLogger> logger;
        NiceMock<MockEventSink> eventSink;
        InputEventCodec codec;
        Camera camera;
        PathIndex paths;
        antwika::game::UiOverlay overlay{antwika::game::kUiCanvas};
        antwika::game::MenuState menuState;

        antwika::game::GameSummary run(
            ReplaySource &source, bool readsMenuState)
        {
            antwika::game::GameConfig config{
                .logger = logger,
                .eventSink = eventSink,
                .inputSource = source,
                .codec = codec,
                .extent = kExtent,
                .camera = camera,
                .paths = paths,
                .maxTicks = 10,
                .overlay = overlay};
            if (readsMenuState)
            {
                config.menuState = menuState;
            }

            return antwika::game::bootstrap(config);
        }
    };

    // What the menu looks like with nothing under the pointer.
    // A key press says nothing about where a pointer is.
    [[nodiscard]] antwika::ui::DrawList menuPicture(
        const antwika::game::MenuState &state)
    {
        return antwika::game::MainMenu{}
            .describe(antwika::game::kUiCanvas, antwika::ui::Pointer{},
                      state)
            .commands;
    }
} // namespace

// The menu, through the front door: nothing but F10 reaches it.
TEST(BootstrapMenuTest, Bootstrap_F10PutsTheMenuUpAndPaintsIt)
{
    MenuHarness harness;
    ReplaySource source({
        TickEvent{
            .tick = 0,
            .event = harness.codec.encode(
                antwika::input::KeyPressed{
                    .key = antwika::game::kMenuKey})},
        TickEvent{
            .tick = 1,
            .event = Event{.name = antwika::engine::events::kStop}},
    });

    const auto summary = harness.run(source, true);

    EXPECT_TRUE(harness.menuState.open);
    EXPECT_EQ(
        harness.overlay.commands(),
        menuPicture(antwika::game::MenuState{.open = true}));
}

// The one thing wiring it in has to keep working: the toolbar.
// F10 twice leaves the bar's picture in the overlay, not the menu's.
TEST(BootstrapMenuTest, Bootstrap_F10TwicePutsTheToolbarBack)
{
    MenuHarness harness;
    ReplaySource source({
        TickEvent{
            .tick = 0,
            .event = harness.codec.encode(
                antwika::input::KeyPressed{
                    .key = antwika::game::kMenuKey})},
        TickEvent{
            .tick = 1,
            .event = harness.codec.encode(
                antwika::input::KeyPressed{
                    .key = antwika::game::kMenuKey})},
        TickEvent{
            .tick = 2,
            .event = Event{.name = antwika::engine::events::kStop}},
    });

    harness.run(source, true);

    EXPECT_FALSE(harness.menuState.open);
    EXPECT_EQ(
        harness.overlay.commands(),
        antwika::game::Toolbar{}
            .describe(
                antwika::game::kUiCanvas,
                antwika::ui::Pointer{},
                harness.camera)
            .commands);
}

// The intent reaches the caller that asked for it.
TEST(BootstrapMenuTest, Bootstrap_ReportsWhatWasActivatedToItsCaller)
{
    MenuHarness harness;
    const auto pixel = menuPixelOn(
        antwika::game::menuWidgets::kSaveReplay,
        antwika::game::MenuState{.open = true});
    ASSERT_GE(pixel.x, 0);

    ReplaySource source({
        TickEvent{
            .tick = 0,
            .event = harness.codec.encode(
                antwika::input::KeyPressed{
                    .key = antwika::game::kMenuKey})},
        TickEvent{
            .tick = 0,
            .event = harness.codec.encode(
                antwika::input::PointerButtonPressed{
                    .button = antwika::input::MouseButton::Left,
                    .position = pixel})},
        TickEvent{
            .tick = 0,
            .event = Event{.name = antwika::engine::events::kStop}},
    });

    const auto summary = harness.run(source, true);

    // Saving leaves the session where it was, so the menu stays up.
    EXPECT_TRUE(harness.menuState.open);
    EXPECT_EQ(
        antwika::game::MenuEntry::SaveReplay, harness.menuState.activated);

    // And the click was the menu's, not the world's.
    EXPECT_TRUE(summary.paths.empty());
}

// A caller wanting no intents still gets a menu it can open.
TEST(BootstrapMenuTest, Bootstrap_WiresAMenuEvenWithNoStateToReportTo)
{
    MenuHarness harness;
    ReplaySource source({
        TickEvent{
            .tick = 0,
            .event = harness.codec.encode(
                antwika::input::KeyPressed{
                    .key = antwika::game::kMenuKey})},
        TickEvent{
            .tick = 1,
            .event = Event{.name = antwika::engine::events::kStop}},
    });

    harness.run(source, false);

    // The state it was handed is untouched, and a menu is up anyway.
    EXPECT_FALSE(harness.menuState.open);
    EXPECT_EQ(
        harness.overlay.commands(),
        menuPicture(antwika::game::MenuState{.open = true}));
}
