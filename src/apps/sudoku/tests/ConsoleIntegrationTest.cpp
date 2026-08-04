#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include <antwika/console/ConsolePicture.hpp>
#include <antwika/console/ConsoleState.hpp>
#include <antwika/console/testing/ConsoleScript.hpp>
#include <antwika/event/mocks/MockEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/i18n/Locale.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/replay/ReplaySource.hpp>
#include <antwika/testing/ScratchPath.hpp>

#include <antwika/app/TickLimitSource.hpp>
#include <antwika/sudoku/Board.hpp>
#include <antwika/sudoku/PuzzleFile.hpp>
#include <antwika/sudoku/PuzzleSource.hpp>
#include <antwika/sudoku/PuzzleState.hpp>
#include <antwika/sudoku/Status.hpp>
#include <antwika/sudoku/Sudoku.hpp>
#include <antwika/sudoku/SudokuScene.hpp>

#include "antwika/sudoku/Messages.hpp"
#include "WidgetCentre.hpp"

using antwika::app::TickLimitSource;
using antwika::console::kConsoleAnimTicks;
using antwika::console::testing::keyAt;
using antwika::console::testing::kOpenTick;
using antwika::console::testing::pressAt;
using antwika::console::testing::typeText;
using antwika::event::mocks::MockEventSink;
using antwika::event::TickEvent;
using antwika::gfx::Point;
using antwika::gfx::Size;
using antwika::input::InputEventCodec;
using antwika::input::Key;
using antwika::log::mocks::MockLogger;
using antwika::replay::ReplaySource;
using antwika::sudoku::Board;
using antwika::sudoku::kDemoPuzzle;
using antwika::sudoku::PuzzleSource;
using antwika::sudoku::PuzzleState;
using antwika::sudoku::Square;
using antwika::sudoku::Status;
using antwika::sudoku::SudokuScene;
using antwika::sudoku::SudokuSummary;
using antwika::sudoku::tests::squareCentre;
using antwika::time::Tick;
using ::testing::NiceMock;
using ::testing::StartsWith;

namespace
{
    constexpr antwika::sudoku::Translator kTranslator{
        antwika::i18n::kDefaultLocale};

    constexpr Size kCanvas{.width = 720, .height = 800};
    constexpr Tick kMaxTicks = 60;

    // The demo puzzle's one blank square.
    constexpr Square kBlank{.row = 0, .col = 2};

    // Where that blank square sits in the flat summary grid.
    constexpr std::size_t kBlankIndex = 2;

    [[nodiscard]] Point placeOf(const Square square)
    {
        const SudokuScene scene{kTranslator};
        PuzzleState state;
        state.start(Board::parse(kDemoPuzzle));

        const auto found = squareCentre(
            scene.describe(kCanvas, {}, state), square);
        EXPECT_TRUE(found.has_value());
        return found.value_or(Point{});
    }

    // One session over a script, with the console mounted.
    [[nodiscard]] SudokuSummary play(
        std::vector<TickEvent> events,
        Tick stopTick,
        const std::string &dumpPath,
        bool loadEnabled = true)
    {
        NiceMock<MockLogger> logger;
        NiceMock<MockEventSink> sink;
        const InputEventCodec codec;
        antwika::console::ConsolePicture consoleOverlay{kCanvas};

        ReplaySource file(std::move(events));
        PuzzleSource puzzled(file, Board::parse(kDemoPuzzle));
        TickLimitSource source(puzzled, stopTick);

        return antwika::sudoku::bootstrap({
            .logger = logger,
            .eventSink = sink,
            .inputSource = source,
            .codec = codec,
            .translator = kTranslator,
            .canvas = kCanvas,
            .maxTicks = kMaxTicks,
            .consoleOverlay = consoleOverlay,
            .consoleLoadEnabled = loadEnabled,
            .stateDumpPath = dumpPath});
    }

    TEST(ConsoleIntegrationTest, AnUnknownCommandIsEchoedAndRefused)
    {
        const InputEventCodec codec;
        std::vector<TickEvent> events{keyAt(codec, 1, Key::Grave)};
        typeText(events, codec, kOpenTick, "hello");
        events.push_back(keyAt(codec, kOpenTick, Key::Enter));

        const auto summary =
            play(std::move(events), kOpenTick + 1, "unused.json");

        EXPECT_EQ(
            summary.console,
            (std::vector<std::string>{
                "> hello", "unknown command: hello"}));
    }

    TEST(ConsoleIntegrationTest, ADigitTypesIntoTheOpenConsole)
    {
        const InputEventCodec codec;

        // The square is picked before the console comes out.
        // So the 4 has somewhere to land, were it ever let through.
        std::vector<TickEvent> events{
            pressAt(codec, 1, placeOf(kBlank)),
            keyAt(codec, 2, Key::Grave)};
        events.push_back(keyAt(codec, kOpenTick + 1, Key::Digit4));
        events.push_back(keyAt(codec, kOpenTick + 1, Key::Enter));

        const auto summary =
            play(std::move(events), kOpenTick + 2, "unused.json");

        // The console read the 4 and the picked square never did.
        EXPECT_EQ(summary.grid.at(kBlankIndex), '.');
        EXPECT_EQ(
            summary.console,
            (std::vector<std::string>{"> 4", "unknown command: 4"}));
    }

    TEST(ConsoleIntegrationTest, APressUnderTheSheetSelectsNothing)
    {
        const InputEventCodec codec;

        // Open, press a square the sheet stands over, close again.
        std::vector<TickEvent> events{keyAt(codec, 1, Key::Grave)};
        events.push_back(pressAt(codec, kOpenTick, placeOf(kBlank)));
        events.push_back(keyAt(codec, kOpenTick + 1, Key::Grave));

        // With the console away, a digit lands in the picked square.
        // There must be none, so the blank has to stay blank.
        const Tick closed = kOpenTick + 1 + kConsoleAnimTicks;
        events.push_back(keyAt(codec, closed, Key::Digit4));

        const auto summary =
            play(std::move(events), closed + 1, "unused.json");

        EXPECT_EQ(summary.grid.at(kBlankIndex), '.');
        EXPECT_EQ(summary.status, Status::Playing);
    }

    TEST(ConsoleIntegrationTest, DumpThenLoadRoundTripsMidGame)
    {
        const antwika::testing::ScratchFile file{
            "antwika_sudoku_console_dump_"};
        const InputEventCodec codec;

        // Pick the blank square, write a 4, and dump the session.
        std::vector<TickEvent> events{
            pressAt(codec, 1, placeOf(kBlank)),
            keyAt(codec, 2, Key::Digit4),
            keyAt(codec, 3, Key::Grave)};
        typeText(events, codec, 3 + kConsoleAnimTicks, "dump_state");
        events.push_back(
            keyAt(codec, 3 + kConsoleAnimTicks, Key::Enter));

        const auto dumped = play(
            std::move(events),
            3 + kConsoleAnimTicks + 1,
            file.string());

        EXPECT_EQ(dumped.grid.at(kBlankIndex), '4');
        EXPECT_EQ(
            dumped.console,
            (std::vector<std::string>{
                "> dump_state",
                "dumped state to " + file.string()}));

        // A fresh session on the same puzzle loads it back.
        std::vector<TickEvent> loading{keyAt(codec, 1, Key::Grave)};
        typeText(loading, codec, kOpenTick, "load_state");
        loading.push_back(keyAt(codec, kOpenTick, Key::Enter));

        const auto loaded = play(
            std::move(loading), kOpenTick + 1, file.string());

        // The grid is the dumped one again, 4 included.
        // And the history is what the dumped console read.
        EXPECT_EQ(loaded.grid.at(kBlankIndex), '4');
        EXPECT_EQ(
            loaded.console,
            (std::vector<std::string>{
                "> dump_state",
                "dumped state to " + file.string(),
                "loaded state from " + file.string()}));
    }

    TEST(ConsoleIntegrationTest, ALoadIsRefusedWhileRecording)
    {
        const InputEventCodec codec;
        std::vector<TickEvent> events{keyAt(codec, 1, Key::Grave)};
        typeText(events, codec, kOpenTick, "load_state");
        events.push_back(keyAt(codec, kOpenTick, Key::Enter));

        const auto summary = play(
            std::move(events), kOpenTick + 1, "unused.json", false);

        EXPECT_EQ(
            summary.console,
            (std::vector<std::string>{
                "> load_state",
                "load_state: not available while recording or "
                "replaying"}));
    }

    TEST(ConsoleIntegrationTest, ALoadAnswersAMissingFile)
    {
        const InputEventCodec codec;
        std::vector<TickEvent> events{keyAt(codec, 1, Key::Grave)};
        typeText(events, codec, kOpenTick, "load_state");
        events.push_back(keyAt(codec, kOpenTick, Key::Enter));

        const auto summary = play(
            std::move(events),
            kOpenTick + 1,
            antwika::testing::scratchPath(
                "antwika_sudoku_console_missing_")
                .string());

        ASSERT_EQ(summary.console.size(), 2U);
        EXPECT_EQ(summary.console.at(0), "> load_state");
        EXPECT_THAT(
            summary.console.at(1), StartsWith("could not load: "));
    }

    TEST(ConsoleIntegrationTest, NoConsoleMountedMeansNoConsole)
    {
        NiceMock<MockLogger> logger;
        NiceMock<MockEventSink> sink;
        const InputEventCodec codec;

        // The very script that opens and types at a mounted console.
        std::vector<TickEvent> events{keyAt(codec, 1, Key::Grave)};
        typeText(events, codec, kOpenTick, "hello");
        events.push_back(keyAt(codec, kOpenTick, Key::Enter));

        ReplaySource file(std::move(events));
        PuzzleSource puzzled(file, Board::parse(kDemoPuzzle));
        TickLimitSource source(puzzled, kOpenTick + 1);

        const auto summary = antwika::sudoku::bootstrap({
            .logger = logger,
            .eventSink = sink,
            .inputSource = source,
            .codec = codec,
            .translator = kTranslator,
            .canvas = kCanvas,
            .maxTicks = kMaxTicks});

        EXPECT_TRUE(summary.console.empty());
    }
} // namespace
