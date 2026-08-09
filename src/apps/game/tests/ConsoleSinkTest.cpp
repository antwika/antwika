#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
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
#include <antwika/simulation/EngineLoopError.hpp>
#include <antwika/testing/ScratchPath.hpp>
#include <antwika/console/ConsolePicture.hpp>
#include <antwika/console/SnapshotFormat.hpp>
#include <antwika/console/ConsoleScene.hpp>
#include <antwika/console/ConsoleSink.hpp>
#include <antwika/console/ConsoleState.hpp>
#include <antwika/console/IConsoleCommands.hpp>
#include <antwika/console/IConsoleControls.hpp>
#include <antwika/console/InputFold.hpp>
#include <antwika/console/conformance/ConsoleContractTest.hpp>
#include <antwika/console/conformance/ConsoleSnapshotRoundTripTest.hpp>
#include <antwika/console/testing/ConsoleScript.hpp>

#include "Translators.hpp"
#include "WidgetCentre.hpp"
#include "antwika/game/AppMode.hpp"
#include "antwika/game/BuildingIndex.hpp"
#include "antwika/game/Camera.hpp"
#include "antwika/game/MainMenuScene.hpp"
#include "antwika/game/Desirability.hpp"
#include "antwika/game/Events.hpp"
#include "antwika/game/Game.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/IsoProjection.hpp"
#include "antwika/game/KeyboardEvent.hpp"
#include "antwika/game/KeyboardLayout.hpp"
#include "antwika/game/LocaleState.hpp"
#include "antwika/game/MapView.hpp"
#include "antwika/game/OptionsState.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/PauseState.hpp"
#include "antwika/game/RoadDrag.hpp"
#include "antwika/game/SessionStore.hpp"
#include "antwika/game/StateDump.hpp"
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
using antwika::console::kConsoleAnimTicks;
using antwika::console::testing::keyAt;
using antwika::console::testing::kOpenTick;
using antwika::console::testing::moveTo;
using antwika::console::testing::pressAt;
using antwika::console::testing::releaseAt;
using antwika::console::testing::scrollAt;
using antwika::console::testing::stopAt;
using antwika::console::testing::typeText;
using antwika::input::InputEventCodec;
using antwika::input::Key;
using antwika::input::KeyPressed;
using antwika::input::MouseButton;
using antwika::log::mocks::MockLogger;
using antwika::replay::ReplaySource;
using antwika::simulation::EngineLoopError;
using antwika::time::Tick;
using ::testing::NiceMock;
using ::testing::StartsWith;

namespace
{
    constexpr GridExtent kExtent{.width = 16, .height = 16};

    struct ReadDump final
    {
        antwika::game::StateDump state;
        std::vector<std::string> console;

        [[nodiscard]] bool operator==(
            const ReadDump &other) const = default;
    };

    [[nodiscard]] std::size_t ticksIn(const TickEventRecorder &recorder)
    {
        return static_cast<std::size_t>(std::ranges::count_if(
            recorder.getEvents(),
            [](const TickEvent &event)
            {
                return event.event.name
                       == antwika::engine::events::kTick;
            }));
    }

    [[nodiscard]] ReadDump readDump(const std::string &path)
    {
        const antwika::console::SnapshotFormat format(
            {.magic = antwika::game::kStateDumpMagic,
             .version = antwika::game::kStateDumpVersion},
            "antwika game state dump document",
            antwika::game::standardStateDumpMigrations);

        const auto snapshot = format.read(path);

        return ReadDump{
            .state = antwika::game::stateDumpFromJson(snapshot.state),
            .console = snapshot.console};
    }

    struct ConsoleHarness final
    {
        explicit ConsoleHarness(AppMode start = AppMode::CityMap)
            : mode{start}
        {
        }

        NiceMock<MockLogger> logger;
        NiceMock<MockEventSink> eventSink;
        InputEventCodec codec;
        Camera camera;
        antwika::game::PathIndex paths;
        antwika::game::BuildingIndex built;
        AppModeState mode;
        antwika::game::PauseState pause;
        antwika::game::RoadDrag drag;
        antwika::game::MapViewState mapView;
        antwika::game::DesirabilityField desirability;
        antwika::game::UiOverlay menuOverlay{antwika::game::kUiCanvas};
        antwika::console::ConsolePicture consoleOverlay{
            antwika::game::kUiCanvas};

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
                .menuOverlay = menuOverlay,
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

    struct GameConsoleTraits final
    {
        using Summary = antwika::game::GameSummary;

        static Summary run(
            std::vector<TickEvent> script,
            const std::string &dumpPath,
            const bool loadEnabled)
        {
            script.push_back(stopAt(kOpenTick + 1));

            ReplaySource source(std::move(script));
            ConsoleHarness harness;

            return harness.run(source, 40, dumpPath, loadEnabled);
        }

        static const std::vector<std::string> &console(
            const Summary &summary)
        {
            return summary.console;
        }

        static void expectUntouched(const Summary &summary)
        {
            EXPECT_TRUE(summary.paths.empty());
        }

        static std::string scratchPrefix()
        {
            return "antwika_game_console.";
        }
    };
}

namespace antwika::console::conformance
{

    INSTANTIATE_TYPED_TEST_SUITE_P(
        Game, ConsoleContractTest, GameConsoleTraits);

    INSTANTIATE_TYPED_TEST_SUITE_P(
        Game, ConsoleSnapshotRoundTripTest, GameConsoleTraits);

}

TEST(ConsoleSinkTest, Run_GivesAnOpenConsoleTheCitysKeys)
{
    ConsoleHarness harness;
    std::vector<TickEvent> events{keyAt(harness.codec, 1, Key::Grave)};

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

TEST(ConsoleSinkTest, Run_ClosingTheConsoleGivesTheKeysBack)
{
    ConsoleHarness harness;
    std::vector<TickEvent> events{
        keyAt(harness.codec, 1, Key::Grave),
        keyAt(harness.codec, kOpenTick, Key::Grave)};

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

TEST(ConsoleSinkTest, Run_APressUnderTheSheetLaysNothing)
{
    ConsoleHarness harness;

    const Cell under{.x = 2, .y = 2};
    const Cell below{.x = 12, .y = 12};
    const auto underPoint = antwika::game::cellCentre(under, Camera());
    const auto belowPoint = antwika::game::cellCentre(below, Camera());

    const auto sheet = static_cast<std::int32_t>(
        antwika::game::kUiCanvas.height / 2);
    ASSERT_LT(underPoint.y, sheet);
    ASSERT_GT(belowPoint.y, sheet);

    std::vector<TickEvent> events{keyAt(harness.codec, 1, Key::Grave)};
    events.push_back(pressAt(harness.codec, kOpenTick, underPoint));
    events.push_back(releaseAt(harness.codec, kOpenTick, underPoint));
    events.push_back(pressAt(harness.codec, kOpenTick, belowPoint));
    events.push_back(releaseAt(harness.codec, kOpenTick, belowPoint));

    events.push_back(pressAt(
        harness.codec, kOpenTick, underPoint, MouseButton::Right));
    events.push_back(stopAt(kOpenTick + 1));
    ReplaySource source(std::move(events));

    const auto summary = harness.run(source, 40, "unused.json");

    EXPECT_EQ(summary.paths, (std::vector<Cell>{below}));
}

TEST(ConsoleSinkTest, Run_AScrollUnderTheSheetZoomsNothing)
{
    ConsoleHarness harness;

    std::vector<TickEvent> events{keyAt(harness.codec, 1, Key::Grave)};

    events.push_back(
        moveTo(harness.codec, kOpenTick, {.x = 100, .y = 100}));
    events.push_back(scrollAt(harness.codec, kOpenTick, 1));
    events.push_back(stopAt(kOpenTick + 1));
    ReplaySource source(std::move(events));

    const auto summary = harness.run(source, 40, "unused.json");

    EXPECT_EQ(
        summary.camera.zoomLevel(), antwika::game::kDefaultZoomLevel);
}

TEST(ConsoleSinkTest, Run_AScrollBelowTheSheetStillZooms)
{
    ConsoleHarness harness;

    std::vector<TickEvent> events{keyAt(harness.codec, 1, Key::Grave)};
    events.push_back(
        moveTo(harness.codec, kOpenTick, {.x = 100, .y = 400}));
    events.push_back(scrollAt(harness.codec, kOpenTick, 1));
    events.push_back(stopAt(kOpenTick + 1));
    ReplaySource source(std::move(events));

    const auto summary = harness.run(source, 40, "unused.json");

    EXPECT_NE(
        summary.camera.zoomLevel(), antwika::game::kDefaultZoomLevel);
}

TEST(ConsoleSinkTest, Run_ExecutesOnlyOnAFreshEnter)
{
    ConsoleHarness harness;
    std::vector<TickEvent> events{keyAt(harness.codec, 1, Key::Grave)};
    typeText(events, harness.codec, kOpenTick, "helloo");
    events.push_back(keyAt(harness.codec, kOpenTick, Key::Backspace));

    events.push_back(keyAt(
        harness.codec,
        kOpenTick,
        KeyPressed{.key = Key::Enter, .repeat = true}));
    events.push_back(keyAt(harness.codec, kOpenTick, Key::Enter));
    events.push_back(stopAt(kOpenTick + 1));
    ReplaySource source(std::move(events));

    const auto summary = harness.run(source, 40, "unused.json");

    EXPECT_EQ(
        summary.console,
        (std::vector<std::string>{
            "> hello", "unknown command: hello"}));
}

TEST(ConsoleSinkTest, Run_ADownPaletteSurvivesTheRoundTrip)
{
    const antwika::testing::ScratchFile file(
        "antwika_game_console_palette.json");
    const auto path = file.path().string();

    const Cell ground{.x = 12, .y = 12};
    const auto point = antwika::game::cellCentre(ground, Camera());

    {
        ConsoleHarness harness;
        std::vector<TickEvent> events{
            pressAt(harness.codec, 1, point, MouseButton::Right),
            keyAt(harness.codec, 2, Key::Grave)};
        typeText(
            events, harness.codec, 2 + kConsoleAnimTicks, "dump_state");
        events.push_back(
            keyAt(harness.codec, 2 + kConsoleAnimTicks, Key::Enter));
        events.push_back(stopAt(3 + kConsoleAnimTicks));
        ReplaySource source(std::move(events));

        harness.run(source, 40, path);
    }

    const auto dumped = readDump(path);
    ASSERT_EQ(dumped.state.tool, std::nullopt);

    ConsoleHarness fresh;
    std::vector<TickEvent> events{keyAt(fresh.codec, 1, Key::Grave)};
    typeText(events, fresh.codec, kOpenTick, "load_state");
    events.push_back(keyAt(fresh.codec, kOpenTick, Key::Enter));
    events.push_back(keyAt(fresh.codec, kOpenTick, Key::Grave));

    const Tick closed = kOpenTick + kConsoleAnimTicks + 1;
    events.push_back(pressAt(fresh.codec, closed, point));
    events.push_back(stopAt(closed + 1));
    ReplaySource source(std::move(events));

    const auto summary = fresh.run(source, 60, path);

    EXPECT_EQ(summary.paths, dumped.state.save.paths);
}

TEST(ConsoleSinkTest, Run_TheConsoleOpensOverTheMainMenu)
{
    ConsoleHarness harness{AppMode::MainMenu};
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

TEST(ConsoleSinkTest, Run_HidesWhatTheSheetCoversFromTheMenu)
{
    const antwika::game::MainMenuScene scene{
        antwika::game::tests::kTranslator};
    const auto frame =
        scene.describe(antwika::game::kUiCanvas, antwika::ui::Pointer{});

    const auto newGame = antwika::game::tests::widgetCentre(
        frame, antwika::game::menuWidgets::kNewGame);
    const auto quit = antwika::game::tests::widgetCentre(
        frame, antwika::game::menuWidgets::kQuit);
    ASSERT_TRUE(newGame.has_value());
    ASSERT_TRUE(quit.has_value());

    const auto sheet = static_cast<std::int32_t>(
        antwika::game::kUiCanvas.height / 2);
    ASSERT_LT(newGame->y, sheet);
    ASSERT_GT(quit->y, sheet);

    ConsoleHarness harness{AppMode::MainMenu};

    std::vector<TickEvent> events{keyAt(harness.codec, 1, Key::Grave)};

    events.push_back(pressAt(harness.codec, kOpenTick, *newGame));
    events.push_back(pressAt(harness.codec, kOpenTick, *quit));
    events.push_back(stopAt(kOpenTick + 5));
    ReplaySource source(std::move(events));

    const auto summary = harness.run(source, 40, "unused.json");

    EXPECT_EQ(summary.state.ticksProcessed, kOpenTick + 1);
    EXPECT_TRUE(summary.console.empty());
}

TEST(ConsoleSinkTest, Run_DumpStateWorksFromTheMainMenu)
{
    const antwika::testing::ScratchFile file(
        "antwika_game_console_menu_dump.json");
    const auto path = file.path().string();

    ConsoleHarness harness{AppMode::MainMenu};
    std::vector<TickEvent> events{keyAt(harness.codec, 1, Key::Grave)};
    typeText(events, harness.codec, kOpenTick, "dump_state");
    events.push_back(keyAt(harness.codec, kOpenTick, Key::Enter));
    events.push_back(stopAt(kOpenTick + 1));
    ReplaySource source(std::move(events));

    const auto summary = harness.run(source, 40, path);

    const auto dumped = readDump(path);

    EXPECT_TRUE(dumped.state.save.paths.empty());
    EXPECT_EQ(summary.console, dumped.console);
}

TEST(ConsoleSinkTest, Run_TypesByTheAnnouncedBoard)
{
    {
        ConsoleHarness harness;
        std::vector<TickEvent> events{
            keyAt(harness.codec, 1, Key::Grave)};
        events.push_back(keyAt(harness.codec, kOpenTick, Key::Minus));
        events.push_back(keyAt(harness.codec, kOpenTick, Key::Enter));
        events.push_back(stopAt(kOpenTick + 1));
        ReplaySource source(std::move(events));

        const auto summary = harness.run(source, 40, "unused.json");

        EXPECT_EQ(
            summary.console,
            (std::vector<std::string>{"> +", "unknown command: +"}));
    }

    {
        ConsoleHarness harness;
        std::vector<TickEvent> events{
            TickEvent{
                .tick = 1,
                .event =
                    Event{
                        .name = antwika::game::events::kSetKeyboard,
                        .payload = antwika::game::setKeyboardPayload(
                            antwika::game::KeyboardLayout::English)}},
            keyAt(harness.codec, 1, Key::Grave)};
        events.push_back(keyAt(harness.codec, kOpenTick, Key::Minus));
        events.push_back(keyAt(harness.codec, kOpenTick, Key::Enter));
        events.push_back(stopAt(kOpenTick + 1));
        ReplaySource source(std::move(events));

        const auto summary = harness.run(source, 40, "unused.json");

        EXPECT_EQ(
            summary.console,
            (std::vector<std::string>{"> -", "unknown command: -"}));
        EXPECT_EQ(
            summary.keyboard, antwika::game::KeyboardLayout::English);
    }
}

TEST(ConsoleSinkTest, Run_AnEmptyLineExecutesNothing)
{
    ConsoleHarness harness;
    std::vector<TickEvent> events{keyAt(harness.codec, 1, Key::Grave)};
    events.push_back(keyAt(harness.codec, kOpenTick, Key::Space));
    events.push_back(keyAt(harness.codec, kOpenTick, Key::Enter));
    events.push_back(stopAt(kOpenTick + 1));
    ReplaySource source(std::move(events));

    const auto summary = harness.run(source, 40, "unused.json");

    EXPECT_TRUE(summary.console.empty());
}

TEST(ConsoleSinkTest, Run_QuitEndsTheRunLongBeforeTheTickLimit)
{
    ConsoleHarness harness;
    std::vector<TickEvent> events{keyAt(harness.codec, 1, Key::Grave)};
    typeText(events, harness.codec, kOpenTick, "quit");
    events.push_back(keyAt(harness.codec, kOpenTick, Key::Enter));
    ReplaySource source(std::move(events));

    TickEventRecorder recorder;

    (void)harness.run(source, 40, "unused.json", true, &recorder);

    EXPECT_EQ(ticksIn(recorder), kOpenTick + 1);
}

TEST(ConsoleSinkTest, Run_ACommandThatIsNotQuitLeavesTheRunGoing)
{
    ConsoleHarness harness;
    std::vector<TickEvent> events{keyAt(harness.codec, 1, Key::Grave)};
    typeText(events, harness.codec, kOpenTick, "quiz");
    events.push_back(keyAt(harness.codec, kOpenTick, Key::Enter));
    ReplaySource source(std::move(events));

    EXPECT_THROW(
        (void)harness.run(source, 40, "unused.json"), EngineLoopError);
}

TEST(ConsoleSinkTest, Run_DumpStateWritesTheInstantAndSaysSo)
{
    const antwika::testing::ScratchFile file(
        "antwika_game_console_dump.json");
    const auto path = file.path().string();

    ConsoleHarness harness;
    const Cell laid{.x = 12, .y = 12};
    const auto point = antwika::game::cellCentre(laid, Camera());

    std::vector<TickEvent> events{
        pressAt(harness.codec, 1, point),
        releaseAt(harness.codec, 1, point),
        keyAt(harness.codec, 2, Key::Grave)};
    typeText(
        events, harness.codec, 2 + kConsoleAnimTicks, "dump_state");
    events.push_back(
        keyAt(harness.codec, 2 + kConsoleAnimTicks, Key::Enter));
    events.push_back(stopAt(3 + kConsoleAnimTicks));
    ReplaySource source(std::move(events));

    const auto summary = harness.run(source, 40, path);

    const auto dumped = readDump(path);

    EXPECT_EQ(dumped.state.save.paths, (std::vector<Cell>{laid}));
    EXPECT_FALSE(dumped.state.paused);
    EXPECT_EQ(dumped.state.tool, antwika::game::BuildTool::Road);
    EXPECT_EQ(dumped.state.view, antwika::game::MapView::Normal);
    EXPECT_EQ(dumped.state.locale, antwika::i18n::kDefaultLocale);
    EXPECT_EQ(
        dumped.console,
        (std::vector<std::string>{
            "> dump_state", "dumped state to " + path}));
    EXPECT_EQ(summary.console, dumped.console);
}

TEST(ConsoleSinkTest, Run_LoadStateComesBackToTheDumpedInstant)
{
    const antwika::testing::ScratchFile file(
        "antwika_game_console_load.json");
    const auto path = file.path().string();

    {
        ConsoleHarness harness;
        const auto point = antwika::game::cellCentre(
            Cell{.x = 12, .y = 12}, Camera());

        std::vector<TickEvent> events{
            pressAt(harness.codec, 1, point),
            keyAt(harness.codec, 2, Key::Grave)};
        typeText(
            events, harness.codec, 2 + kConsoleAnimTicks, "dump_state");
        events.push_back(
            keyAt(harness.codec, 2 + kConsoleAnimTicks, Key::Enter));
        events.push_back(stopAt(3 + kConsoleAnimTicks));
        ReplaySource source(std::move(events));

        harness.run(source, 40, path);
    }

    const auto dumped = readDump(path);

    ConsoleHarness fresh;
    std::vector<TickEvent> events{keyAt(fresh.codec, 1, Key::Grave)};
    typeText(events, fresh.codec, kOpenTick, "load_state");
    events.push_back(keyAt(fresh.codec, kOpenTick, Key::Enter));
    events.push_back(stopAt(kOpenTick + 1));
    ReplaySource source(std::move(events));

    const auto summary = fresh.run(source, 40, path);

    EXPECT_EQ(summary.paths, dumped.state.save.paths);
    EXPECT_EQ(summary.state.money, dumped.state.save.state.money);
    EXPECT_EQ(summary.camera, dumped.state.save.camera);
}

TEST(ConsoleSinkTest, LoadState_AnswersARefusedState)
{
    const antwika::testing::ScratchFile file(
        "antwika_game_console_load_bad_state.json");
    const auto path = file.path().string();

    const antwika::console::SnapshotFormat format(
        {.magic = antwika::game::kStateDumpMagic,
         .version = antwika::game::kStateDumpVersion},
        "antwika game state dump document",
        antwika::game::standardStateDumpMigrations);
    const antwika::game::StateDump dump;
    auto state = antwika::game::stateDumpToJson(dump);
    state["tool"] = "not-a-tool";
    format.write(
        antwika::console::Snapshot{.console = {}, .state = state},
        path);

    ConsoleHarness harness;
    std::vector<TickEvent> events{keyAt(harness.codec, 1, Key::Grave)};
    typeText(events, harness.codec, kOpenTick, "load_state");
    events.push_back(keyAt(harness.codec, kOpenTick, Key::Enter));
    events.push_back(stopAt(kOpenTick + 1));
    ReplaySource source(std::move(events));

    const auto summary = harness.run(source, 40, path);

    ASSERT_EQ(summary.console.size(), 2U);
    EXPECT_EQ(summary.console[0], "> load_state");
    EXPECT_THAT(summary.console[1], StartsWith("could not load: "));
    EXPECT_TRUE(summary.paths.empty());
}

TEST(ConsoleSinkTest, Run_ReplaysAConsoleSessionIdentically)
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

    std::erase_if(
        recorded,
        [](const TickEvent &event)
        {
            return event.event.name == antwika::engine::events::kTick;
        });

    const auto liveDump = readDump(path);

    ASSERT_FALSE(recorded.empty());
    ASSERT_FALSE(live.console.empty());

    ConsoleHarness again;
    ReplaySource source(recorded);

    const auto replayed = again.run(source, 40, path, false);

    EXPECT_EQ(replayed, live);
    EXPECT_EQ(readDump(path), liveDump);
}

namespace
{
    struct FakeQuietCommands final : antwika::console::IConsoleCommands
    {
        void execute(
            const std::string &, antwika::console::ConsoleState &)
            override
        {
        }

        [[nodiscard]] std::vector<std::string> names() const override
        {
            return {};
        }
    };

    struct SinkHarness final
    {
        InputEventCodec codec;
        antwika::console::InputFold input{codec};
        antwika::console::ConsolePicture picture{
            antwika::game::kUiCanvas};
        antwika::console::ConsoleScene scene;
        antwika::console::ConsoleState console;
        antwika::console::FixedConsoleControls controls;
        FakeQuietCommands commands;
        antwika::console::ConsoleSink sink{
            antwika::console::ConsoleSinkSetup{
                .console = console,
                .input = input,
                .picture = picture,
                .scene = scene,
                .controls = controls,
                .commands = commands}};

        void feed(const TickEvent &event)
        {
            input.handle(event);
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
            feed(keyAt(codec, 1, Key::Grave));

            for (Tick tick = 1; tick <= kConsoleAnimTicks; ++tick)
            {
                feedTick(tick);
            }
        }
    };
}

TEST(ConsoleSinkTest, Run_TakesAToggleRepeatAsHeld)
{
    SinkHarness harness;
    harness.openFully();

    harness.feed(keyAt(
        harness.codec,
        kConsoleAnimTicks + 1,
        KeyPressed{.key = Key::Grave, .repeat = true}));
    harness.feedTick(kConsoleAnimTicks + 1);

    EXPECT_TRUE(harness.console.acceptsText());
}
