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

#include "TestTranslator.hpp"
#include "WidgetPixel.hpp"
#include "antwika/game/AppMode.hpp"
#include "antwika/game/BuildingIndex.hpp"
#include "antwika/game/Camera.hpp"
#include "antwika/game/MainMenuScene.hpp"
#include <antwika/console/ConsolePicture.hpp>
#include <antwika/console/SnapshotFormat.hpp>
#include <antwika/console/ConsoleScene.hpp>
#include <antwika/console/ConsoleSink.hpp>
#include <antwika/console/ConsoleState.hpp>
#include <antwika/console/IConsoleCommands.hpp>
#include <antwika/console/IConsoleControls.hpp>
#include <antwika/console/InputFold.hpp>
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
    // A run types by the Swedish board unless told otherwise.
    // So the underscore is shift over the American slash position.
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
                events.push_back(keyAt(codec, tick, Key::Slash, true));
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

    // A dump read back through the game's own envelope.
    struct ReadDump
    {
        antwika::game::StateDump state;
        std::vector<std::string> console;

        [[nodiscard]] bool operator==(
            const ReadDump &other) const = default;
    };

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

    [[nodiscard]] TickEvent stopAt(Tick tick)
    {
        return TickEvent{
            .tick = tick,
            .event = Event{.name = antwika::engine::events::kStop}};
    }

    // BootstrapTest's harness, with the console turned on.
    struct ConsoleHarness
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

    const auto dumped = readDump(path);
    ASSERT_EQ(dumped.state.tool, std::nullopt);

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
    EXPECT_EQ(summary.paths, dumped.state.save.paths);
}

TEST(ConsoleSinkTest, TheConsoleOpensOverTheMainMenu)
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

TEST(ConsoleSinkTest, WhatTheSheetCoversTheMenuNeverSees)
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

    // One button under the sheet and one below it.
    const auto sheet = static_cast<std::int32_t>(
        antwika::game::kUiCanvas.height / 2);
    ASSERT_LT(newGame->y, sheet);
    ASSERT_GT(quit->y, sheet);

    ConsoleHarness harness{AppMode::MainMenu};

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

    // New Game under the sheet starts nothing.
    // Quit below it still quits, which is what ends this run.
    events.push_back(pressAt(kOpenTick, *newGame));
    events.push_back(pressAt(kOpenTick, *quit));
    events.push_back(stopAt(kOpenTick + 5));
    ReplaySource source(std::move(events));

    const auto summary = harness.run(source, 40, "unused.json");

    EXPECT_EQ(summary.state.ticksProcessed, kOpenTick + 1);
    EXPECT_TRUE(summary.console.empty());
}

TEST(ConsoleSinkTest, DumpStateWorksFromTheMainMenu)
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

    // The dump is the live session, whichever screen was up.
    const auto dumped = readDump(path);

    EXPECT_TRUE(dumped.state.save.paths.empty());
    EXPECT_EQ(summary.console, dumped.console);
}

TEST(ConsoleSinkTest, TheAnnouncedBoardDecidesWhatAKeyTypes)
{
    // By default the American minus position prints a Swedish plus.
    {
        ConsoleHarness harness;
        std::vector<TickEvent> events{
            keyAt(harness.codec, 1, Key::Grave)};
        events.push_back(keyAt(harness.codec, kOpenTick, Key::Comma));
        events.push_back(keyAt(harness.codec, kOpenTick, Key::Enter));
        events.push_back(stopAt(kOpenTick + 1));
        ReplaySource source(std::move(events));

        const auto summary = harness.run(source, 40, "unused.json");

        EXPECT_EQ(
            summary.console,
            (std::vector<std::string>{"> ,", "unknown command: ,"}));
    }

    // Announced English, the same position types by that board.
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
        events.push_back(keyAt(harness.codec, kOpenTick, Key::Comma));
        events.push_back(keyAt(harness.codec, kOpenTick, Key::Enter));
        events.push_back(stopAt(kOpenTick + 1));
        ReplaySource source(std::move(events));

        const auto summary = harness.run(source, 40, "unused.json");

        // The American comma position types nothing this build maps.
        // So the field stays empty and Enter says nothing at all.
        EXPECT_TRUE(summary.console.empty());
        EXPECT_EQ(
            summary.keyboard, antwika::game::KeyboardLayout::English);
    }
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

    const auto dumped = readDump(path);

    // A fresh session loads it and continues from that instant.
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

TEST(ConsoleSinkTest, LoadStateAnswersAStateTheDecoderRefuses)
{
    const antwika::testing::ScratchFile file(
        "antwika_game_console_load_bad_state.json");
    const auto path = file.path().string();

    // A well-enveloped dump whose state names an unknown tool.
    // The decoder's SaveFormatError comes back as the answer.
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

    const auto liveDump = readDump(path);

    // The replay re-executes the dump and rewrites the same file.
    ConsoleHarness again;
    ReplaySource source(recorded);

    const auto replayed = again.run(source, 40, path, false);

    EXPECT_EQ(replayed, live);
    EXPECT_EQ(readDump(path), liveDump);
}

namespace
{
    // A command set that swallows everything, for the shell alone.
    struct QuietCommands final : antwika::console::IConsoleCommands
    {
        void execute(
            const std::string &, antwika::console::ConsoleState &)
            override
        {
        }
    };

    // The sink alone, for the branches a whole run cannot steer.
    struct SinkHarness
    {
        InputEventCodec codec;
        antwika::console::InputFold input{codec};
        antwika::console::ConsolePicture picture{
            antwika::game::kUiCanvas};
        antwika::console::ConsoleScene scene;
        antwika::console::ConsoleState console;
        antwika::console::FixedConsoleControls controls;
        QuietCommands commands;
        antwika::console::ConsoleSink sink{
            antwika::console::ConsoleSinkSetup{
                .console = console,
                .input = input,
                .picture = picture,
                .scene = scene,
                .controls = controls,
                .commands = commands}};

        // The fold first, then the sink.
        // The order bootstrap() registers the two in.
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
