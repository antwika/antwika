#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <sstream>

#include <vector>

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/ecs/mocks/MockSystem.hpp>
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
#include <antwika/simulation/EngineLoopError.hpp>
#include <antwika/replay/ReplaySource.hpp>


#include "Translators.hpp"
#include "WidgetCentre.hpp"
#include "antwika/game/AppMode.hpp"
#include "antwika/game/BuildingIndex.hpp"
#include "antwika/game/Camera.hpp"
#include "antwika/game/Events.hpp"
#include "antwika/game/Game.hpp"
#include "antwika/game/LocaleState.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Direction.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/IsoProjection.hpp"
#include "antwika/game/MainMenuScene.hpp"
#include "antwika/game/MenuItem.hpp"
#include "antwika/game/UiCanvas.hpp"
#include "antwika/game/UiOverlay.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/Ruin.hpp"
#include "antwika/game/Desirability.hpp"
#include "antwika/game/MapView.hpp"
#include "antwika/game/PauseState.hpp"
#include "antwika/game/RoadDrag.hpp"
#include "antwika/game/BuildTool.hpp"
#include "antwika/game/SaveGame.hpp"
#include "antwika/game/Toolbar.hpp"
#include "antwika/game/WorldMap.hpp"
#include "antwika/game/WorldMapLayout.hpp"
#include "antwika/game/WorldMapSink.hpp"
#include "antwika/game/WorldMapState.hpp"

using antwika::ecs::mocks::MockSystem;
using antwika::game::tests::kTranslator;

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
using antwika::simulation::EngineLoopError;
using antwika::replay::ReplaySource;
using ::testing::_;
using ::testing::NiceMock;

namespace
{
    constexpr GridExtent kExtent{.width = 16, .height = 16};

    struct Harness final
    {
        NiceMock<MockLogger> logger;
        NiceMock<MockEventSink> eventSink;
        InputEventCodec codec;
        Camera camera;
        PathIndex paths;
        antwika::game::BuildingIndex built;

        AppModeState mode{AppMode::CityMap};
        antwika::game::PauseState pause;

        antwika::game::RoadDrag drag;

        antwika::game::MapViewState mapView;
        antwika::game::DesirabilityField desirability;

        antwika::game::GameSummary run(
            ReplaySource &source,
            antwika::time::Tick maxTicks,
            antwika::event::ITickEventSink *recorder = nullptr)
        {
            antwika::game::GameWiring config{
                .logger = logger,
                .eventSink = eventSink,
                .inputSource = source,
                .codec = codec,
                .extent = kExtent,
                .camera = camera,
                .paths = paths,
                .built = built,
                .mode = mode,
                .pause = pause,
                .view = mapView,
                .desirability = desirability,
                .drag = drag,
                .maxTicks = maxTicks};
            if (recorder != nullptr)
            {
                config.replayRecorder = *recorder;
            }

            return antwika::game::bootstrap(config);
        }
    };
}

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

TEST(BootstrapTest, Bootstrap_ThrowsWhenMaxTicksIsReachedWithoutAStopEvent)
{
    Harness harness;
    ReplaySource source({});

    EXPECT_THROW(harness.run(source, 3), EngineLoopError);
}

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

    const auto releaseAt = [&codec](antwika::game::Cell cell,
                                    antwika::input::MouseButton button)
    {
        const auto point =
            antwika::game::cellCentre(cell, antwika::game::Camera());
        return codec.encode(
            antwika::input::PointerButtonReleased{
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
            .event = releaseAt(
                antwika::game::Cell{.x = 2, .y = 1},
                antwika::input::MouseButton::Left)},
        TickEvent{
            .tick = 0,
            .event = pressAt(
                antwika::game::Cell{.x = 1, .y = 1},
                antwika::input::MouseButton::Right)},
        TickEvent{
            .tick = 0,
            .event = pressAt(
                antwika::game::Cell{.x = 1, .y = 1},
                antwika::input::MouseButton::Right)},
        TickEvent{
            .tick = 0,
            .event = Event{.name = antwika::engine::events::kStop}},
    };
    ReplaySource source(script);

    const auto summary = harness.run(source, 10);

    EXPECT_EQ(summary.paths.size(), 2U);
    ASSERT_EQ(summary.walkers.size(), 1U);

    EXPECT_EQ(summary.walkers[0].at, (antwika::game::Cell{.x = 2, .y = 1}));
    EXPECT_EQ(summary.walkers[0].facing, antwika::game::Direction::East);
}

namespace
{
}

TEST(BootstrapTest, Bootstrap_RunsEveryObserverOncePerTick)
{
    Harness harness;
    ReplaySource source({
        TickEvent{
            .tick = 2,
            .event = Event{.name = antwika::engine::events::kStop},
        },
    });

    NiceMock<MockSystem> first;
    EXPECT_CALL(first, update(_, _)).Times(3U);
    NiceMock<MockSystem> second;
    EXPECT_CALL(second, update(_, _)).Times(3U);

    antwika::game::bootstrap(
        antwika::game::GameWiring{
            .logger = harness.logger,
            .eventSink = harness.eventSink,
            .inputSource = source,
            .codec = harness.codec,
            .extent = kExtent,
            .camera = harness.camera,
            .paths = harness.paths,
            .built = harness.built,
            .mode = harness.mode,
            .pause = harness.pause,
            .observers = {first, second},
            .maxTicks = 10});
}

TEST(PrintSummaryTest, PrintSummary_WritesTheStateAndCamera)
{
    std::ostringstream out;
    const antwika::game::GameSummary summary{
        .state = {.ticksProcessed = 4, .score = 7},
        .paths = {{.x = 1, .y = 1}, {.x = 1, .y = 2}},
        .walkers = {},
        .buildings = {},
        .ruins =
            {{.at = {.x = 2, .y = 3},
              .kind = antwika::game::BuildingKind::Farm,
              .state = antwika::game::RuinState::Debris}},
        .camera = Camera(Point{.x = 512, .y = 48}),
        .ratings = {},
        .console = {},
        .bindings = {}};

    antwika::game::printSummary(out, summary);

    EXPECT_EQ(
        out.str(),
        "Final state: ticksProcessed=4 score=7 money=5000\n"
        "Paths laid: 2\n"
        "Walkers: 0\n"
        "Buildings: 0\n"
        "Ruins: 1\n"
        "  farm at (2, 3) debris\n"
        "Ratings: population=0 employment=0 housing=0 service=0\n"
        "Console lines: 0\n"
        "Camera: pan (512, 48) zoom 3\n");
}

TEST(PrintSummaryTest, PrintSummary_WritesEveryWalkersPose)
{
    std::ostringstream out;
    const antwika::game::GameSummary summary{
        .state = {},
        .paths = {},
        .walkers =
            {{.at = {.x = 3, .y = 4},
              .facing = antwika::game::Direction::South}},
        .buildings = {},
        .ruins = {},
        .camera = Camera(Point{.x = 0, .y = 0}),
        .ratings = {},
        .console = {},
        .bindings = {}};

    antwika::game::printSummary(out, summary);

    EXPECT_NE(
        out.str().find("  at (3, 4) facing 2\n"), std::string::npos);
}

namespace
{
    [[nodiscard]] antwika::input::Position menuPixelOn(
        antwika::ui::WidgetId id)
    {
        const antwika::game::MainMenuScene scene{kTranslator};

        const auto centre = antwika::game::tests::widgetCentre(
            scene.describe(antwika::game::kUiCanvas, antwika::ui::Pointer{}),
            id);

        if (!centre.has_value())
        {
            return antwika::input::Position{};
        }

        return antwika::input::Position{.x = centre->x, .y = centre->y};
    }

    struct MenuHarness final
    {
        NiceMock<MockLogger> logger;
        NiceMock<MockEventSink> eventSink;
        InputEventCodec codec;
        Camera camera;
        PathIndex paths;
        antwika::game::BuildingIndex built;
        AppModeState mode;
        antwika::game::PauseState pause;
        antwika::game::UiOverlay menuOverlay{antwika::game::kUiCanvas};

        antwika::game::GameSummary run(ReplaySource &source)
        {
            return antwika::game::bootstrap(
                antwika::game::GameWiring{
                    .logger = logger,
                    .eventSink = eventSink,
                    .inputSource = source,
                    .codec = codec,
                    .extent = kExtent,
                    .camera = camera,
                    .paths = paths,
                    .built = built,
                    .mode = mode,
                    .pause = pause,
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

    [[nodiscard]] TickEvent leftReleaseAt(
        const InputEventCodec &codec,
        antwika::time::Tick tick,
        antwika::input::Position at)
    {
        return TickEvent{
            .tick = tick,
            .event = codec.encode(
                antwika::input::PointerButtonReleased{
                    .button = antwika::input::MouseButton::Left,
                    .position = at})};
    }
}

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

    EXPECT_EQ(harness.mode.mode(), antwika::game::AppMode::CityMap);
}

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
        leftReleaseAt(
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

    EXPECT_NO_THROW(harness.run(source));
}

TEST(BootstrapTest, Bootstrap_WordsItselfWithTheTranslatorItIsGiven)
{
    Harness harness;
    const InputEventCodec codec;

    ReplaySource source(
        {TickEvent{
            .tick = 1,
            .event = Event{.name = antwika::engine::events::kStop}}});

    NiceMock<MockLogger> logger;
    NiceMock<MockEventSink> eventSink;
    antwika::game::UiOverlay overlay{antwika::game::kUiCanvas};

    const auto summary = antwika::game::bootstrap(
        antwika::game::GameWiring{
            .logger = logger,
            .eventSink = eventSink,
            .inputSource = source,
            .codec = codec,
            .extent = kExtent,
            .camera = harness.camera,
            .paths = harness.paths,
            .built = harness.built,
            .mode = harness.mode,
            .pause = harness.pause,
            .maxTicks = 4,
            .overlay = overlay});

    EXPECT_EQ(summary.state.ticksProcessed, 2U);

    const antwika::game::Toolbar toolbar{kTranslator};
    const auto expected = toolbar.describe(
        antwika::game::kUiCanvas,
        antwika::ui::Pointer{},
        harness.camera,
        antwika::game::BuildTool::Road,
        false,
        summary.state.ticksProcessed - 1);

    EXPECT_EQ(overlay.commands(), expected.commands);
}

TEST(BootstrapTest, Bootstrap_TheDemoReplaysOpeningClickHitsNewGame)
{
    const antwika::game::MainMenuScene scene{kTranslator};
    const antwika::ui::Pointer pointer{
        .position = antwika::gfx::Point{.x = 500, .y = 250}};

    EXPECT_EQ(
        scene.describe(antwika::game::kUiCanvas, pointer)
            .interactions.hovered,
        antwika::game::menuWidgets::kNewGame);
}

namespace
{
    constexpr antwika::game::WorldMapConfig kWorld{
        .width = 16, .height = 12, .seed = 11};

    struct WorldHarness final
    {
        NiceMock<MockLogger> logger;
        NiceMock<MockEventSink> eventSink;
        InputEventCodec codec;
        Camera camera;
        PathIndex paths;
        antwika::game::BuildingIndex built;
        AppModeState mode;
        antwika::game::PauseState pause;
        antwika::game::UiOverlay menuOverlay{antwika::game::kUiCanvas};
        antwika::game::WorldMapState cities{
            antwika::game::generateWorldMap(kWorld)};

        antwika::game::GameSummary run(ReplaySource &source)
        {
            return antwika::game::bootstrap(
                antwika::game::GameWiring{
                    .logger = logger,
                    .eventSink = eventSink,
                    .inputSource = source,
                    .codec = codec,
                    .extent = kExtent,
                    .camera = camera,
                    .paths = paths,
                    .built = built,
                    .mode = mode,
                    .pause = pause,
                    .maxTicks = 20,
                    .menuOverlay = menuOverlay,
                    .world = cities});
        }

        [[nodiscard]] antwika::input::Position cityPixel(std::size_t city)
        {
            const auto rect = antwika::game::worldTileRect(
                antwika::game::kUiCanvas,
                cities.world().width,
                cities.world().height,
                cities.world().cityCell(city));
            return antwika::input::Position{
                .x = rect.origin.x + 1, .y = rect.origin.y + 1};
        }
    };
}

TEST(BootstrapTest, Bootstrap_PressingWorldMapLeavesTheMenuForTheWorld)
{
    WorldHarness harness;
    const InputEventCodec codec;

    ReplaySource source({
        leftPressAt(
            codec, 0, menuPixelOn(antwika::game::menuWidgets::kWorldMap)),
        TickEvent{
            .tick = 2,
            .event = Event{.name = antwika::engine::events::kStop}},
    });

    harness.run(source);

    EXPECT_EQ(harness.mode.mode(), antwika::game::AppMode::WorldMap);
}

TEST(BootstrapTest, Bootstrap_OpeningACityLaysNothingOnItsGrid)
{
    WorldHarness harness;
    const InputEventCodec codec;

    ReplaySource source({
        leftPressAt(
            codec, 0, menuPixelOn(antwika::game::menuWidgets::kWorldMap)),
        leftPressAt(codec, 1, harness.cityPixel(2)),
        TickEvent{
            .tick = 3,
            .event = Event{.name = antwika::engine::events::kStop}},
    });

    const auto summary = harness.run(source);

    EXPECT_EQ(harness.mode.mode(), antwika::game::AppMode::CityMap);
    EXPECT_EQ(harness.cities.city(), 2U);
    EXPECT_TRUE(summary.paths.empty());
}

TEST(BootstrapTest, Bootstrap_BuildsInTheCityItOpenedAndKeepsItOnTheWayBack)
{
    WorldHarness harness;
    const InputEventCodec codec;
    const auto centre = antwika::game::cellCentre(
        antwika::game::Cell{.x = 2, .y = 3}, Camera());

    ReplaySource source({
        leftPressAt(
            codec, 0, menuPixelOn(antwika::game::menuWidgets::kWorldMap)),
        leftPressAt(codec, 1, harness.cityPixel(1)),
        leftPressAt(
            codec,
            2,
            antwika::input::Position{.x = centre.x, .y = centre.y}),
        leftReleaseAt(
            codec,
            2,
            antwika::input::Position{.x = centre.x, .y = centre.y}),
        TickEvent{
            .tick = 3,
            .event = codec.encode(
                antwika::input::KeyPressed{
                    .key = antwika::game::kWorldMapKey})},
        TickEvent{
            .tick = 5,
            .event = Event{.name = antwika::engine::events::kStop}},
    });

    harness.run(source);

    EXPECT_EQ(harness.mode.mode(), antwika::game::AppMode::WorldMap);
    EXPECT_FALSE(harness.cities.cityOpen());
    EXPECT_TRUE(
        harness.cities.cityPaths(1).has(antwika::game::Cell{.x = 2, .y = 3}));
    EXPECT_FALSE(
        harness.cities.cityPaths(0).has(antwika::game::Cell{.x = 2, .y = 3}));
}

namespace
{
    struct SaveHarness final
    {
        NiceMock<MockLogger> logger;
        NiceMock<MockEventSink> eventSink;
        InputEventCodec codec;
        Camera camera;
        PathIndex paths;
        antwika::game::BuildingIndex built;
        AppModeState mode;
        antwika::game::PauseState pause;
        antwika::game::UiOverlay menuOverlay{antwika::game::kUiCanvas};
        antwika::game::UiOverlay saveOverlay{antwika::game::kUiCanvas};

        antwika::game::GameSummary run(
            ReplaySource &source,
            std::optional<antwika::game::SaveGame> start = std::nullopt)
        {
            return antwika::game::bootstrap(
                antwika::game::GameWiring{
                    .logger = logger,
                    .eventSink = eventSink,
                    .inputSource = source,
                    .codec = codec,
                    .extent = kExtent,
                    .camera = camera,
                    .paths = paths,
                    .built = built,
                    .mode = mode,
                    .pause = pause,
                    .maxTicks = 20,
                    .menuOverlay = menuOverlay,
                    .saveOverlay = saveOverlay,
                    .start = std::move(start)});
        }
    };
}

TEST(BootstrapTest, Bootstrap_PressingLoadGameOpensTheSaveScreen)
{
    SaveHarness harness;
    const InputEventCodec codec;

    ReplaySource source({
        leftPressAt(
            codec, 0, menuPixelOn(antwika::game::menuWidgets::kLoadGame)),
        TickEvent{
            .tick = 2,
            .event = Event{.name = antwika::engine::events::kStop}},
    });

    harness.run(source);

    EXPECT_EQ(harness.mode.mode(), antwika::game::AppMode::SaveLoad);
}

TEST(BootstrapTest, Bootstrap_StartsFromASaveWhenGivenOne)
{
    SaveHarness harness;
    antwika::game::SaveGame start;
    start.paths = {{.x = 5, .y = 6}, {.x = 6, .y = 6}};
    start.state = GameState{.ticksProcessed = 0, .score = 4};
    start.camera = Camera(Point{.x = 7, .y = 8}, 1);

    ReplaySource source({
        TickEvent{
            .tick = 1,
            .event = Event{.name = antwika::engine::events::kStop}},
    });

    const auto summary = harness.run(source, start);

    EXPECT_EQ(summary.paths, start.paths);
    EXPECT_EQ(summary.camera, start.camera);
    EXPECT_EQ(summary.state.score, 4U);
}

TEST(PrintSummaryTest, PrintSummary_WritesEveryBuildingAndWhatItIs)
{
    std::ostringstream out;
    const antwika::game::GameSummary summary{
        .state = {},
        .paths = {},
        .walkers = {},
        .buildings =
            {{.at = {.x = 1, .y = 2},
              .kind = antwika::game::BuildingKind::House,
              .coverage = {11, 22},
              .level = antwika::game::HousingLevel::Hovel},
             {.at = {.x = 3, .y = 4},
              .kind = antwika::game::BuildingKind::Well}},
        .ruins = {},
        .camera = Camera(Point{.x = 0, .y = 0}),
        .ratings = {},
        .console = {},
        .bindings = {}};

    antwika::game::printSummary(out, summary);

    EXPECT_NE(out.str().find("Buildings: 2\n"), std::string::npos);

    EXPECT_NE(
        out.str().find(
            "  house at (1, 2) hovel covered water=11 health=22\n"),
        std::string::npos);
    EXPECT_NE(
        out.str().find(
            "  well at (3, 4) tent covered water=0 health=0\n"),
        std::string::npos);
}

TEST(BootstrapTest, Bootstrap_TheDemoReplaysPaletteClickHitsTheHouse)
{
    const antwika::game::Toolbar toolbar{kTranslator};
    const Camera camera;
    const antwika::ui::Pointer pointer{
        .position = antwika::gfx::Point{.x = 968, .y = 108}};

    EXPECT_EQ(
        toolbar.describe(antwika::game::kUiCanvas, pointer, camera)
            .interactions.hovered,
        antwika::game::widgets::toolWidget(
            antwika::game::BuildTool::House));

    EXPECT_EQ(
        antwika::game::screenToCell(
            antwika::gfx::Point{.x = 544, .y = 176},
            Camera(Point{.x = 512, .y = 48})),
        (antwika::game::Cell{.x = 4, .y = 3}));
}

TEST(BootstrapTest, Bootstrap_LaysAWholeRunOfRoadFromOneDrag)
{
    Harness harness;
    const InputEventCodec codec;

    const auto pixelOf = [](antwika::game::Cell cell)
    {
        const auto point =
            antwika::game::cellCentre(cell, antwika::game::Camera());
        return antwika::input::Position{.x = point.x, .y = point.y};
    };

    const std::vector<TickEvent> script{
        TickEvent{
            .tick = 0,
            .event = codec.encode(
                antwika::input::PointerButtonPressed{
                    .button = antwika::input::MouseButton::Left,
                    .position = pixelOf(antwika::game::Cell{.x = 2, .y = 2})})},
        TickEvent{
            .tick = 1,
            .event = codec.encode(
                antwika::input::PointerMoved{
                    .position = pixelOf(antwika::game::Cell{.x = 6, .y = 2})})},
        TickEvent{
            .tick = 2,
            .event = codec.encode(
                antwika::input::PointerButtonReleased{
                    .button = antwika::input::MouseButton::Left,
                    .position = pixelOf(antwika::game::Cell{.x = 6, .y = 2})})},
        TickEvent{
            .tick = 3,
            .event = Event{.name = antwika::engine::events::kStop}},
    };
    ReplaySource source(script);

    const auto summary = harness.run(source, 10);

    EXPECT_EQ(summary.paths.size(), 5U);

    for (std::int32_t x = 2; x <= 6; ++x)
    {
        EXPECT_TRUE(harness.paths.has(antwika::game::Cell{.x = x, .y = 2}));
    }

    EXPECT_FALSE(harness.pause.paused());
}

namespace
{
    [[nodiscard]] antwika::input::Position barPixelOn(
        antwika::ui::WidgetId id, bool menuOpen)
    {
        const antwika::game::Toolbar toolbar{kTranslator};

        const auto centre = antwika::game::tests::widgetCentre(
            toolbar.describe(
                antwika::game::kUiCanvas,
                antwika::ui::Pointer{},
                Camera(),
                antwika::game::BuildTool::Road,
                false,
                0,
                antwika::game::CityRatings{},
                menuOpen),
            id);

        if (!centre.has_value())
        {
            return antwika::input::Position{};
        }

        return antwika::input::Position{.x = centre->x, .y = centre->y};
    }

    struct BarHarness final
    {
        NiceMock<MockLogger> logger;
        NiceMock<MockEventSink> eventSink;
        InputEventCodec codec;
        Camera camera;
        PathIndex paths;
        antwika::game::BuildingIndex built;
        AppModeState mode{AppMode::CityMap};
        antwika::game::PauseState pause;
        antwika::game::UiOverlay overlay{antwika::game::kUiCanvas};
        antwika::game::WorldMapState cities{
            antwika::game::generateWorldMap(kWorld)};

        antwika::game::GameSummary run(ReplaySource &source)
        {
            return antwika::game::bootstrap(
                antwika::game::GameWiring{
                    .logger = logger,
                    .eventSink = eventSink,
                    .inputSource = source,
                    .codec = codec,
                    .extent = kExtent,
                    .camera = camera,
                    .paths = paths,
                    .built = built,
                    .mode = mode,
                    .pause = pause,
                    .maxTicks = 20,
                    .overlay = overlay,
                    .world = cities});
        }
    };
}

TEST(BootstrapTest, Bootstrap_TheBarsGameMenuLeavesTheCityForTheWorld)
{
    BarHarness harness;
    const InputEventCodec codec;

    ReplaySource source({
        leftPressAt(
            codec, 0, barPixelOn(antwika::game::widgets::kGameMenu, false)),
        leftPressAt(
            codec,
            1,
            barPixelOn(
                antwika::game::widgets::menuItemWidget(
                    antwika::game::MenuItem::WorldMap),
                true)),
        TickEvent{
            .tick = 3,
            .event = Event{.name = antwika::engine::events::kStop}},
    });

    harness.run(source);

    EXPECT_EQ(antwika::game::AppMode::WorldMap, harness.mode.mode());
    EXPECT_FALSE(harness.cities.cityOpen());
}

TEST(BootstrapTest, Bootstrap_OpeningTheBarsGameMenuLaysNothing)
{
    BarHarness harness;
    const InputEventCodec codec;

    ReplaySource source({
        leftPressAt(
            codec, 0, barPixelOn(antwika::game::widgets::kGameMenu, false)),
        TickEvent{
            .tick = 2,
            .event = Event{.name = antwika::engine::events::kStop}},
    });

    const auto summary = harness.run(source);

    EXPECT_TRUE(summary.paths.empty());
}

TEST(BootstrapTest, Bootstrap_WordsItselfWithTheLocaleStateItIsGiven)
{
    Harness harness;
    const InputEventCodec codec;

    ReplaySource source(
        {TickEvent{
            .tick = 1,
            .event = Event{.name = antwika::engine::events::kStop}}});

    NiceMock<MockLogger> logger;
    NiceMock<MockEventSink> eventSink;
    antwika::game::UiOverlay overlay{antwika::game::kUiCanvas};

    antwika::game::LocaleState localeState{antwika::i18n::Locale::Swedish};

    const auto summary = antwika::game::bootstrap(
        antwika::game::GameWiring{
            .logger = logger,
            .eventSink = eventSink,
            .inputSource = source,
            .codec = codec,
            .extent = kExtent,
            .camera = harness.camera,
            .paths = harness.paths,
            .built = harness.built,
            .mode = harness.mode,
            .pause = harness.pause,
            .maxTicks = 4,
            .overlay = overlay,
            .locale = localeState});

    EXPECT_EQ(summary.state.ticksProcessed, 2U);

    const antwika::game::Toolbar toolbar{localeState.translator()};
    const auto expected = toolbar.describe(
        antwika::game::kUiCanvas,
        antwika::ui::Pointer{},
        harness.camera,
        antwika::game::BuildTool::Road,
        false,
        summary.state.ticksProcessed - 1);

    EXPECT_EQ(overlay.commands(), expected.commands);

    const antwika::game::Toolbar english{kTranslator};

    EXPECT_NE(
        overlay.commands(),
        english
            .describe(
                antwika::game::kUiCanvas,
                antwika::ui::Pointer{},
                harness.camera,
                antwika::game::BuildTool::Road,
                false,
                summary.state.ticksProcessed - 1)
            .commands);
}
