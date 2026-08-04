#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

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
#include <antwika/replay/ReplaySource.hpp>
#include <antwika/testing/ScratchPath.hpp>

#include "antwika/game/AppMode.hpp"
#include "antwika/game/BuildingIndex.hpp"
#include "antwika/game/Camera.hpp"
#include "antwika/game/ConsoleScene.hpp"
#include "antwika/game/ConsoleSink.hpp"
#include "antwika/game/ConsoleState.hpp"
#include "antwika/game/Desirability.hpp"
#include "antwika/game/Game.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/IsoProjection.hpp"
#include "antwika/game/LocaleState.hpp"
#include "antwika/game/MapView.hpp"
#include "antwika/game/OptionsState.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/PauseState.hpp"
#include "antwika/game/RoadDrag.hpp"
#include "antwika/game/SessionStore.hpp"
#include "antwika/game/StateDumpFile.hpp"
#include "antwika/game/UiCanvas.hpp"
#include "antwika/game/UiOverlay.hpp"

using antwika::event::Event;
using antwika::event::mocks::MockEventSink;
using antwika::event::TickEvent;
using antwika::event::TickEventRecorder;
using antwika::game::AppMode;
using antwika::game::AppModeState;
using antwika::game::Camera;
using antwika::game::Cell;
using antwika::game::GridExtent;
using antwika::game::kConsoleAnimTicks;
using antwika::input::InputEventCodec;
using antwika::input::Key;
using antwika::input::KeyPressed;
using antwika::input::MouseButton;
using antwika::log::mocks::MockLogger;
using antwika::replay::ReplaySource;
using antwika::time::Tick;
using ::testing::NiceMock;
using ::testing::StartsWith;

namespace
{
    constexpr GridExtent kExtent{.width = 16, .height = 16};

    // The first tick on which the field reads.
    // The toggle goes down on tick 1 and each tick slides one step.
    constexpr Tick kOpenTick = 1 + kConsoleAnimTicks;

    [[nodiscard]] TickEvent keyAt(
        const InputEventCodec &codec,
        Tick tick,
        Key key,
        bool shift = false,
        bool repeat = false)
    {
        return TickEvent{
            .tick = tick,
            .event = codec.encode(KeyPressed{
                .key = key,
                .modifiers = {.shift = shift},
                .repeat = repeat})};
    }

    // The keys that type one command, one press per character.
    // Only what the two commands need: letters, underscore, space.
    void typeText(
        std::vector<TickEvent> &events,
        const InputEventCodec &codec,
        Tick tick,
        std::string_view text)
    {
        for (const char character : text)
        {
            if (character == '_')
            {
                events.push_back(keyAt(codec, tick, Key::Minus, true));
                continue;
            }

            if (character == ' ')
            {
                events.push_back(keyAt(codec, tick, Key::Space));
                continue;
            }

            events.push_back(keyAt(
                codec,
                tick,
                static_cast<Key>(
                    static_cast<std::uint8_t>(Key::A)
                    + (character - 'a'))));
        }
    }

    [[nodiscard]] TickEvent stopAt(Tick tick)
    {
        return TickEvent{
            .tick = tick,
            .event = Event{.name = antwika::engine::events::kStop}};
    }

    // BootstrapTest's harness, with the console turned on.
    struct ConsoleHarness
    {
        NiceMock<MockLogger> logger;
        NiceMock<MockEventSink> eventSink;
        InputEventCodec codec;
        Camera camera;
        antwika::game::PathIndex paths;
        antwika::game::BuildingIndex built;
        AppModeState mode{AppMode::CityMap};
        antwika::game::PauseState pause;
        antwika::game::RoadDrag drag;
        antwika::game::MapViewState mapView;
        antwika::game::DesirabilityField desirability;
        antwika::game::UiOverlay consoleOverlay{antwika::game::kUiCanvas};

        antwika::game::GameSummary run(
            ReplaySource &source,
            Tick maxTicks,
            const std::string &dumpPath,
            bool loadEnabled = true,
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
                .maxTicks = maxTicks,
                .consoleOverlay = consoleOverlay,
                .consoleLoadEnabled = loadEnabled,
                .stateDumpPath = dumpPath};
            if (recorder != nullptr)
            {
                config.replayRecorder = *recorder;
            }

            return antwika::game::bootstrap(config);
        }
    };
} // namespace

TEST(ConsoleSinkTest, AnUnknownCommandIsEchoedAndRefused)
{
    ConsoleHarness harness;
    std::vector<TickEvent> events{keyAt(harness.codec, 1, Key::Grave)};
    typeText(events, harness.codec, kOpenTick, "hello");
    events.push_back(keyAt(harness.codec, kOpenTick, Key::Enter));
    events.push_back(stopAt(kOpenTick + 1));
    ReplaySource source(std::move(events));

    const auto summary = harness.run(source, 40, "unused.json");

    EXPECT_EQ(
        summary.console,
        (std::vector<std::string>{
            "> hello", "unknown command: hello"}));
}

TEST(ConsoleSinkTest, TypingBeforeFullyOpenReachesNoField)
{
    ConsoleHarness harness;
    std::vector<TickEvent> events{keyAt(harness.codec, 1, Key::Grave)};

    // Half way along the slide, none of this may land.
    typeText(events, harness.codec, 3, "hello");
    events.push_back(keyAt(harness.codec, 3, Key::Enter));

    // Fully open, the field is still empty, so Enter says nothing.
    events.push_back(keyAt(harness.codec, kOpenTick, Key::Enter));
    events.push_back(stopAt(kOpenTick + 1));
    ReplaySource source(std::move(events));

    const auto summary = harness.run(source, 40, "unused.json");

    EXPECT_TRUE(summary.console.empty());
}

TEST(ConsoleSinkTest, AnOpenConsoleTakesTheKeysTheCityWouldRead)
{
    ConsoleHarness harness;
    std::vector<TickEvent> events{keyAt(harness.codec, 1, Key::Grave)};

    // Equal is bound to zoom in, and Space to pause.
    // Over an open console the first must do nothing at all.
    // And the second must type the space between two words.
    events.push_back(keyAt(harness.codec, kOpenTick, Key::Equal));
    typeText(events, harness.codec, kOpenTick, "a b");
    events.push_back(keyAt(harness.codec, kOpenTick, Key::Enter));
    events.push_back(stopAt(kOpenTick + 1));
    ReplaySource source(std::move(events));

    const auto summary = harness.run(source, 40, "unused.json");

    EXPECT_EQ(
        summary.camera.zoomLevel(), antwika::game::kDefaultZoomLevel);
    EXPECT_EQ(
        summary.console,
        (std::vector<std::string>{"> a b", "unknown command: a b"}));
}

TEST(ConsoleSinkTest, ClosingTheConsoleGivesTheKeysBack)
{
    ConsoleHarness harness;
    std::vector<TickEvent> events{
        keyAt(harness.codec, 1, Key::Grave),
        keyAt(harness.codec, kOpenTick, Key::Grave)};

    // The slide out takes the animation ticks again.
    const Tick closed = kOpenTick + kConsoleAnimTicks + 1;
    events.push_back(keyAt(harness.codec, closed, Key::Minus));
    events.push_back(stopAt(closed + 1));
    ReplaySource source(std::move(events));

    const auto summary = harness.run(source, 60, "unused.json");

    EXPECT_EQ(
        summary.camera.zoomLevel(),
        antwika::game::kDefaultZoomLevel - 1);
    EXPECT_TRUE(summary.console.empty());
}

TEST(ConsoleSinkTest, APressUnderTheSheetLaysNothing)
{
    ConsoleHarness harness;

    const Cell under{.x = 2, .y = 2};
    const Cell below{.x = 12, .y = 12};
    const auto underPoint = antwika::game::cellCentre(under, Camera());
    const auto belowPoint = antwika::game::cellCentre(below, Camera());

    // The sheet reaches half way down the canvas.
    const auto sheet = static_cast<std::int32_t>(
        antwika::game::kUiCanvas.height / 2);
    ASSERT_LT(underPoint.y, sheet);
    ASSERT_GT(belowPoint.y, sheet);

    const auto pressAt = [&harness](Tick tick, antwika::gfx::Point at)
    {
        return TickEvent{
            .tick = tick,
            .event = harness.codec.encode(
                antwika::input::PointerButtonPressed{
                    .button = MouseButton::Left,
                    .position = {.x = at.x, .y = at.y}})};
    };

    std::vector<TickEvent> events{keyAt(harness.codec, 1, Key::Grave)};
    events.push_back(pressAt(kOpenTick, underPoint));
    events.push_back(pressAt(kOpenTick, belowPoint));

    // A right press under the sheet is the console's too.
    // It reaches the field as no press at all, and cancels nothing.
    events.push_back(TickEvent{
        .tick = kOpenTick,
        .event = harness.codec.encode(
            antwika::input::PointerButtonPressed{
                .button = MouseButton::Right,
                .position = {.x = underPoint.x, .y = underPoint.y}})});
    events.push_back(stopAt(kOpenTick + 1));
    ReplaySource source(std::move(events));

    const auto summary = harness.run(source, 40, "unused.json");

    EXPECT_EQ(summary.paths, (std::vector<Cell>{below}));
}

TEST(ConsoleSinkTest, AScrollUnderTheSheetZoomsNothing)
{
    ConsoleHarness harness;

    const auto moveTo = [&harness](Tick tick, std::int32_t x,
                                   std::int32_t y)
    {
        return TickEvent{
            .tick = tick,
            .event = harness.codec.encode(
                antwika::input::PointerMoved{
                    .position = {.x = x, .y = y}})};
    };

    const auto scrollAt = [&harness](Tick tick)
    {
        return TickEvent{
            .tick = tick,
            .event = harness.codec.encode(
                antwika::input::PointerScrolled{.vertical = 1})};
    };

    std::vector<TickEvent> events{keyAt(harness.codec, 1, Key::Grave)};

    // Under the sheet the wheel is the console's, and does nothing.
    events.push_back(moveTo(kOpenTick, 100, 100));
    events.push_back(scrollAt(kOpenTick));
    events.push_back(stopAt(kOpenTick + 1));
    ReplaySource source(std::move(events));

    const auto summary = harness.run(source, 40, "unused.json");

    EXPECT_EQ(
        summary.camera.zoomLevel(), antwika::game::kDefaultZoomLevel);
}

TEST(ConsoleSinkTest, AScrollBelowTheSheetStillZooms)
{
    ConsoleHarness harness;

    std::vector<TickEvent> events{keyAt(harness.codec, 1, Key::Grave)};
    events.push_back(TickEvent{
        .tick = kOpenTick,
        .event = harness.codec.encode(antwika::input::PointerMoved{
            .position = {.x = 100, .y = 400}})});
    events.push_back(TickEvent{
        .tick = kOpenTick,
        .event = harness.codec.encode(
            antwika::input::PointerScrolled{.vertical = 1})});
    events.push_back(stopAt(kOpenTick + 1));
    ReplaySource source(std::move(events));

    const auto summary = harness.run(source, 40, "unused.json");

    EXPECT_NE(
        summary.camera.zoomLevel(), antwika::game::kDefaultZoomLevel);
}

TEST(ConsoleSinkTest, BackspaceEditsAndOnlyAFreshEnterExecutes)
{
    ConsoleHarness harness;
    std::vector<TickEvent> events{keyAt(harness.codec, 1, Key::Grave)};
    typeText(events, harness.codec, kOpenTick, "helloo");
    events.push_back(keyAt(harness.codec, kOpenTick, Key::Backspace));

    // A held Enter is a held key, and executes nothing at all.
    events.push_back(
        keyAt(harness.codec, kOpenTick, Key::Enter, false, true));
    events.push_back(keyAt(harness.codec, kOpenTick, Key::Enter));
    events.push_back(stopAt(kOpenTick + 1));
    ReplaySource source(std::move(events));

    const auto summary = harness.run(source, 40, "unused.json");

    EXPECT_EQ(
        summary.console,
        (std::vector<std::string>{
            "> hello", "unknown command: hello"}));
}

TEST(ConsoleSinkTest, ADownPaletteSurvivesTheRoundTrip)
{
    const antwika::testing::ScratchFile file(
        "antwika_game_console_palette.json");
    const auto path = file.path().string();

    const Cell ground{.x = 12, .y = 12};
    const auto point = antwika::game::cellCentre(ground, Camera());

    // One session puts the palette down, then dumps itself.
    {
        ConsoleHarness harness;
        std::vector<TickEvent> events{
            TickEvent{
                .tick = 1,
                .event = harness.codec.encode(
                    antwika::input::PointerButtonPressed{
                        .button = MouseButton::Right,
                        .position = {.x = point.x, .y = point.y}})},
            keyAt(harness.codec, 2, Key::Grave)};
        typeText(
            events, harness.codec, 2 + kConsoleAnimTicks, "dump_state");
        events.push_back(
            keyAt(harness.codec, 2 + kConsoleAnimTicks, Key::Enter));
        events.push_back(stopAt(3 + kConsoleAnimTicks));
        ReplaySource source(std::move(events));

        harness.run(source, 40, path);
    }

    const auto dumped = antwika::game::loadStateDump(path);
    ASSERT_EQ(dumped.tool, std::nullopt);

    // A fresh session loads it, closes the console, and clicks.
    ConsoleHarness fresh;
    std::vector<TickEvent> events{keyAt(fresh.codec, 1, Key::Grave)};
    typeText(events, fresh.codec, kOpenTick, "load_state");
    events.push_back(keyAt(fresh.codec, kOpenTick, Key::Enter));
    events.push_back(keyAt(fresh.codec, kOpenTick, Key::Grave));

    const Tick closed = kOpenTick + kConsoleAnimTicks + 1;
    events.push_back(TickEvent{
        .tick = closed,
        .event = fresh.codec.encode(
            antwika::input::PointerButtonPressed{
                .button = MouseButton::Left,
                .position = {.x = point.x, .y = point.y}})});
    events.push_back(stopAt(closed + 1));
    ReplaySource source(std::move(events));

    const auto summary = fresh.run(source, 60, path);

    // The loaded palette is down, so the click laid nothing.
    EXPECT_EQ(summary.paths, dumped.save.paths);
}

TEST(ConsoleSinkTest, AnEmptyLineExecutesNothing)
{
    ConsoleHarness harness;
    std::vector<TickEvent> events{keyAt(harness.codec, 1, Key::Grave)};
    events.push_back(keyAt(harness.codec, kOpenTick, Key::Space));
    events.push_back(keyAt(harness.codec, kOpenTick, Key::Enter));
    events.push_back(stopAt(kOpenTick + 1));
    ReplaySource source(std::move(events));

    const auto summary = harness.run(source, 40, "unused.json");

    // A line of spaces is an empty line, echoed nowhere.
    EXPECT_TRUE(summary.console.empty());
}

TEST(ConsoleSinkTest, DumpStateWritesTheInstantAndSaysSo)
{
    const antwika::testing::ScratchFile file(
        "antwika_game_console_dump.json");
    const auto path = file.path().string();

    ConsoleHarness harness;
    const Cell laid{.x = 12, .y = 12};
    const auto point = antwika::game::cellCentre(laid, Camera());

    std::vector<TickEvent> events{
        TickEvent{
            .tick = 1,
            .event = harness.codec.encode(
                antwika::input::PointerButtonPressed{
                    .button = MouseButton::Left,
                    .position = {.x = point.x, .y = point.y}})},
        keyAt(harness.codec, 2, Key::Grave)};
    typeText(events, harness.codec, 2 + kConsoleAnimTicks, "dump_state");
    events.push_back(keyAt(harness.codec, 2 + kConsoleAnimTicks, Key::Enter));
    events.push_back(stopAt(3 + kConsoleAnimTicks));
    ReplaySource source(std::move(events));

    const auto summary = harness.run(source, 40, path);

    const auto dumped = antwika::game::loadStateDump(path);

    EXPECT_EQ(dumped.save.paths, (std::vector<Cell>{laid}));
    EXPECT_FALSE(dumped.paused);
    EXPECT_EQ(dumped.tool, antwika::game::BuildTool::Road);
    EXPECT_EQ(dumped.view, antwika::game::MapView::Normal);
    EXPECT_EQ(dumped.locale, antwika::i18n::kDefaultLocale);
    EXPECT_EQ(
        dumped.console,
        (std::vector<std::string>{
            "> dump_state", "dumped state to " + path}));
    EXPECT_EQ(summary.console, dumped.console);
}

TEST(ConsoleSinkTest, LoadStateComesBackToTheDumpedInstant)
{
    const antwika::testing::ScratchFile file(
        "antwika_game_console_load.json");
    const auto path = file.path().string();

    // One session lays a road, pans nothing, and dumps itself.
    {
        ConsoleHarness harness;
        const auto point = antwika::game::cellCentre(
            Cell{.x = 12, .y = 12}, Camera());

        std::vector<TickEvent> events{
            TickEvent{
                .tick = 1,
                .event = harness.codec.encode(
                    antwika::input::PointerButtonPressed{
                        .button = MouseButton::Left,
                        .position = {.x = point.x, .y = point.y}})},
            keyAt(harness.codec, 2, Key::Grave)};
        typeText(
            events, harness.codec, 2 + kConsoleAnimTicks, "dump_state");
        events.push_back(
            keyAt(harness.codec, 2 + kConsoleAnimTicks, Key::Enter));
        events.push_back(stopAt(3 + kConsoleAnimTicks));
        ReplaySource source(std::move(events));

        harness.run(source, 40, path);
    }

    const auto dumped = antwika::game::loadStateDump(path);

    // A fresh session loads it and continues from that instant.
    ConsoleHarness fresh;
    std::vector<TickEvent> events{keyAt(fresh.codec, 1, Key::Grave)};
    typeText(events, fresh.codec, kOpenTick, "load_state");
    events.push_back(keyAt(fresh.codec, kOpenTick, Key::Enter));
    events.push_back(stopAt(kOpenTick + 1));
    ReplaySource source(std::move(events));

    const auto summary = fresh.run(source, 40, path);

    EXPECT_EQ(summary.paths, dumped.save.paths);
    EXPECT_EQ(summary.state.money, dumped.save.state.money);
    EXPECT_EQ(summary.camera, dumped.save.camera);

    // The dump's own exchange, then what loading it said.
    auto expected = dumped.console;
    expected.push_back("loaded state from " + path);
    EXPECT_EQ(summary.console, expected);
}

TEST(ConsoleSinkTest, LoadStateIsRefusedWhileRecordingOrReplaying)
{
    ConsoleHarness harness;
    std::vector<TickEvent> events{keyAt(harness.codec, 1, Key::Grave)};
    typeText(events, harness.codec, kOpenTick, "load_state");
    events.push_back(keyAt(harness.codec, kOpenTick, Key::Enter));
    events.push_back(stopAt(kOpenTick + 1));
    ReplaySource source(std::move(events));

    const auto summary =
        harness.run(source, 40, "unused.json", false);

    EXPECT_EQ(
        summary.console,
        (std::vector<std::string>{
            "> load_state",
            "load_state: not available while recording or replaying"}));
    EXPECT_TRUE(summary.paths.empty());
}

TEST(ConsoleSinkTest, LoadStateAnswersAFileThatIsNotThere)
{
    const antwika::testing::ScratchFile file(
        "antwika_game_console_load_absent.json");

    ConsoleHarness harness;
    std::vector<TickEvent> events{keyAt(harness.codec, 1, Key::Grave)};
    typeText(events, harness.codec, kOpenTick, "load_state");
    events.push_back(keyAt(harness.codec, kOpenTick, Key::Enter));
    events.push_back(stopAt(kOpenTick + 1));
    ReplaySource source(std::move(events));

    const auto summary =
        harness.run(source, 40, file.path().string());

    ASSERT_EQ(summary.console.size(), 2U);
    EXPECT_EQ(summary.console[0], "> load_state");
    EXPECT_THAT(summary.console[1], StartsWith("could not load: "));
}

TEST(ConsoleSinkTest, ARecordedConsoleSessionReplaysToTheSameRun)
{
    const antwika::testing::ScratchFile file(
        "antwika_game_console_replay_dump.json");
    const auto path = file.path().string();

    std::vector<TickEvent> recorded;
    antwika::game::GameSummary live;

    {
        ConsoleHarness harness;
        std::vector<TickEvent> events{
            keyAt(harness.codec, 1, Key::Grave)};
        typeText(
            events, harness.codec, kOpenTick, "dump_state");
        events.push_back(keyAt(harness.codec, kOpenTick, Key::Enter));
        events.push_back(stopAt(kOpenTick + 1));
        ReplaySource source(std::move(events));
        TickEventRecorder recorder;

        live = harness.run(source, 40, path, false, &recorder);

        recorded = recorder.getEvents();
    }

    // A replay file never holds engine.tick.
    // The engine regenerates it, so ReplayRecorder filters it out.
    // This in-memory recording gets the same filter by hand.
    std::erase_if(
        recorded,
        [](const TickEvent &event)
        {
            return event.event.name == antwika::engine::events::kTick;
        });

    const auto liveDump = antwika::game::loadStateDump(path);

    // The replay re-executes the dump and rewrites the same file.
    ConsoleHarness again;
    ReplaySource source(recorded);

    const auto replayed = again.run(source, 40, path, false);

    EXPECT_EQ(replayed, live);
    EXPECT_EQ(antwika::game::loadStateDump(path), liveDump);
}

TEST(ConsoleSinkTest, ConsoleLoadPermitted_OnlyInAPlainLiveRun)
{
    EXPECT_TRUE(antwika::game::consoleLoadPermitted(false, false));
    EXPECT_FALSE(antwika::game::consoleLoadPermitted(true, false));
    EXPECT_FALSE(antwika::game::consoleLoadPermitted(false, true));
    EXPECT_FALSE(antwika::game::consoleLoadPermitted(true, true));
}

namespace
{
    // The sink alone, for the branches a whole run cannot steer.
    struct SinkHarness
    {
        NiceMock<MockLogger> logger;
        InputEventCodec codec;
        antwika::game::InputFold input{codec};
        antwika::ecs::World world{logger};
        antwika::game::PathIndex paths;
        antwika::game::BuildingIndex built;
        Camera camera;
        antwika::game::GameState state;
        antwika::game::SessionStore session{
            world, paths, built, camera, state, kExtent, 7};
        antwika::game::OptionsState options;
        antwika::game::UiOverlay overlay{antwika::game::kUiCanvas};
        antwika::game::UiOverlay toolbar;
        antwika::game::ConsoleScene scene;
        antwika::game::ConsoleState console;
        antwika::game::PauseState pause;
        antwika::game::MapViewState mapView;
        antwika::game::LocaleState locale;
        AppModeState mode{AppMode::CityMap};
        antwika::game::ConsoleSink sink{
            antwika::game::ConsoleSinkSetup{
                .console = console,
                .mode = mode,
                .options = options,
                .input = input,
                .overlay = overlay,
                .scene = scene,
                .session = session,
                .pause = pause,
                .view = mapView,
                .toolbar = toolbar,
                .locale = locale,
                .loadEnabled = true},
            "unused.json"};

        // The fold first, then the mode, then the sink.
        // The order bootstrap() registers the three in.
        void feed(const TickEvent &event)
        {
            input.handle(event);
            mode.handle(event);
            sink.handle(event);
        }

        void feedTick(Tick tick)
        {
            feed(TickEvent{
                .tick = tick,
                .event =
                    Event{.name = antwika::engine::events::kTick}});
        }

        void openFully()
        {
            feed(TickEvent{
                .tick = 1,
                .event = codec.encode(
                    KeyPressed{.key = Key::Grave})});

            for (Tick tick = 1; tick <= kConsoleAnimTicks; ++tick)
            {
                feedTick(tick);
            }
        }
    };
} // namespace

TEST(ConsoleSinkTest, LeavingTheCityClosesTheConsole)
{
    SinkHarness harness;
    harness.openFully();

    ASSERT_TRUE(harness.console.acceptsText());

    // The mode lands at the tick boundary, and the console follows.
    harness.mode.request(AppMode::MainMenu);
    harness.feedTick(kConsoleAnimTicks + 1);

    EXPECT_FALSE(harness.console.visible());
}

TEST(ConsoleSinkTest, TheToggleOpensNothingOffTheCityScreen)
{
    SinkHarness harness;
    harness.mode.request(AppMode::MainMenu);
    harness.feedTick(1);

    harness.feed(TickEvent{
        .tick = 2,
        .event =
            harness.codec.encode(KeyPressed{.key = Key::Grave})});
    harness.feedTick(2);

    EXPECT_FALSE(harness.console.visible());
}

TEST(ConsoleSinkTest, ARepeatOfTheToggleKeyIsAHeldKeyNotAPress)
{
    SinkHarness harness;
    harness.openFully();

    harness.feed(TickEvent{
        .tick = kConsoleAnimTicks + 1,
        .event = harness.codec.encode(KeyPressed{
            .key = Key::Grave, .repeat = true})});
    harness.feedTick(kConsoleAnimTicks + 1);

    // A held toggle holds the console open rather than flickering it.
    EXPECT_TRUE(harness.console.acceptsText());
}
