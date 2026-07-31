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
#include <antwika/input/MouseButton.hpp>
#include <antwika/input/Position.hpp>
#include <antwika/ui/Pointer.hpp>
#include <antwika/ui/WidgetId.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/replay/EngineLoopError.hpp>
#include <antwika/replay/ReplaySource.hpp>

#include "antwika/game/AppMode.hpp"
#include "antwika/game/Camera.hpp"
#include "antwika/game/Events.hpp"
#include "antwika/game/Game.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Direction.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/IsoProjection.hpp"
#include "antwika/game/MainMenuScene.hpp"
#include "antwika/game/UiCanvas.hpp"
#include "antwika/game/UiOverlay.hpp"
#include "antwika/game/PathIndex.hpp"

using antwika::event::Event;
using antwika::event::mocks::MockEventSink;
using antwika::event::TickEvent;
using antwika::event::TickEventRecorder;
using antwika::game::AppMode;
using antwika::game::AppModeState;
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

        // The subject of these tests is the grid.
        // So a run starts there rather than clicking past the menu.
        AppModeState mode{AppMode::Playing};

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
                .mode = mode,
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
            .mode = harness.mode,
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
        .camera = Camera(Point{.x = 512, .y = 48})};

    antwika::game::printSummary(out, summary);

    EXPECT_EQ(
        out.str(),
        "Final state: ticksProcessed=4 score=7\n"
        "Paths laid: 2\n"
        "Walkers: 0\n"
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
        .camera = Camera(Point{.x = 0, .y = 0})};

    antwika::game::printSummary(out, summary);

    EXPECT_NE(
        out.str().find("  at (3, 4) facing 2\n"), std::string::npos);
}

// The menu is a mode of its own.
// So bootstrap() has to be able to boot into it.
// And nothing on the command line may decide which.
namespace
{
    // Where an item is, is the layout's business.
    [[nodiscard]] antwika::input::Position menuPixelOn(
        antwika::ui::WidgetId id)
    {
        const antwika::game::MainMenuScene scene;

        for (std::int32_t y = 0;
             y < static_cast<std::int32_t>(antwika::game::kUiCanvas.height);
             y += 2)
        {
            for (std::int32_t x = 0;
                 x
                 < static_cast<std::int32_t>(antwika::game::kUiCanvas.width);
                 x += 2)
            {
                const antwika::ui::Pointer pointer{
                    .position = antwika::gfx::Point{.x = x, .y = y}};

                if (scene.describe(antwika::game::kUiCanvas, pointer)
                        .interactions.hovered
                    == id)
                {
                    return antwika::input::Position{.x = x, .y = y};
                }
            }
        }

        return antwika::input::Position{};
    }

    struct MenuHarness
    {
        NiceMock<MockLogger> logger;
        NiceMock<MockEventSink> eventSink;
        InputEventCodec codec;
        Camera camera;
        PathIndex paths;
        AppModeState mode;
        antwika::game::UiOverlay menuOverlay{antwika::game::kUiCanvas};

        antwika::game::GameSummary run(ReplaySource &source)
        {
            return antwika::game::bootstrap(
                antwika::game::GameConfig{
                    .logger = logger,
                    .eventSink = eventSink,
                    .inputSource = source,
                    .codec = codec,
                    .extent = kExtent,
                    .camera = camera,
                    .paths = paths,
                    .mode = mode,
                    .maxTicks = 10,
                    .menuOverlay = menuOverlay});
        }
    };

    [[nodiscard]] TickEvent leftPressAt(
        const InputEventCodec &codec,
        antwika::time::Tick tick,
        antwika::input::Position at)
    {
        return TickEvent{
            .tick = tick,
            .event = codec.encode(
                antwika::input::PointerButtonPressed{
                    .button = antwika::input::MouseButton::Left,
                    .position = at})};
    }
} // namespace

TEST(BootstrapTest, Bootstrap_StartsAtTheMainMenu)
{
    MenuHarness harness;
    ReplaySource source({
        TickEvent{
            .tick = 1,
            .event = Event{.name = antwika::engine::events::kStop}},
    });

    harness.run(source);

    EXPECT_EQ(harness.mode.mode(), antwika::game::AppMode::MainMenu);
}

// A click on the grid while the menu is up is not the grid's click.
TEST(BootstrapTest, Bootstrap_LaysNoPathWhileTheMenuIsUp)
{
    MenuHarness harness;
    const InputEventCodec codec;
    const auto onTheGrid = antwika::input::Position{
        .x = antwika::game::cellCentre(
                 antwika::game::Cell{.x = 1, .y = 1}, harness.camera)
                 .x,
        .y = antwika::game::cellCentre(
                 antwika::game::Cell{.x = 1, .y = 1}, harness.camera)
                 .y};

    ReplaySource source({
        leftPressAt(codec, 0, onTheGrid),
        TickEvent{
            .tick = 1,
            .event = Event{.name = antwika::engine::events::kStop}},
    });

    const auto summary = harness.run(source);

    EXPECT_TRUE(summary.paths.empty());
}

TEST(BootstrapTest, Bootstrap_PressingNewGameLeavesTheMenuForTheGrid)
{
    MenuHarness harness;
    const InputEventCodec codec;

    ReplaySource source({
        leftPressAt(
            codec, 0, menuPixelOn(antwika::game::menuWidgets::kNewGame)),
        TickEvent{
            .tick = 2,
            .event = Event{.name = antwika::engine::events::kStop}},
    });

    harness.run(source);

    EXPECT_EQ(harness.mode.mode(), antwika::game::AppMode::Playing);
}

// And once it has, the grid takes clicks exactly as it always did.
TEST(BootstrapTest, Bootstrap_LaysPathsOnceTheMenuHasBeenLeft)
{
    MenuHarness harness;
    const InputEventCodec codec;
    const auto centre = antwika::game::cellCentre(
        antwika::game::Cell{.x = 1, .y = 1}, harness.camera);

    ReplaySource source({
        leftPressAt(
            codec, 0, menuPixelOn(antwika::game::menuWidgets::kNewGame)),
        leftPressAt(
            codec,
            1,
            antwika::input::Position{.x = centre.x, .y = centre.y}),
        TickEvent{
            .tick = 2,
            .event = Event{.name = antwika::engine::events::kStop}},
    });

    const auto summary = harness.run(source);

    EXPECT_EQ(summary.paths.size(), 1U);
}

TEST(BootstrapTest, Bootstrap_PressingQuitEndsTheRun)
{
    MenuHarness harness;
    const InputEventCodec codec;

    ReplaySource source({
        leftPressAt(
            codec, 0, menuPixelOn(antwika::game::menuWidgets::kQuit)),
    });

    // No engine.stop in the script at all.
    // Reaching maxTicks would throw.
    // So completing the run is what proves the menu stopped it.
    EXPECT_NO_THROW(harness.run(source));
}

// replays/demo.json opens by clicking here.
// A recording is only as good as the layout it was made against.
// So the pixel it holds is asserted here.
// Otherwise it is rediscovered by hand every time an item moves.
TEST(BootstrapTest, Bootstrap_TheDemoReplaysOpeningClickHitsNewGame)
{
    const antwika::game::MainMenuScene scene;
    const antwika::ui::Pointer pointer{
        .position = antwika::gfx::Point{.x = 500, .y = 250}};

    EXPECT_EQ(
        scene.describe(antwika::game::kUiCanvas, pointer)
            .interactions.hovered,
        antwika::game::menuWidgets::kNewGame);
}
