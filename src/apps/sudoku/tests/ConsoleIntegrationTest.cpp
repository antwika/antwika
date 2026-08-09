#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include <antwika/console/ConsolePicture.hpp>
#include <antwika/console/ConsoleState.hpp>
#include <antwika/console/conformance/ConsoleContractTest.hpp>
#include <antwika/console/conformance/ConsoleSnapshotRoundTripTest.hpp>
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

#include "WidgetCentre.hpp"
#include "antwika/sudoku/Messages.hpp"

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

namespace
{
    constexpr antwika::sudoku::Translator kTranslator{
        antwika::i18n::kDefaultLocale};

    constexpr Size kCanvas{.width = 720, .height = 800};
    constexpr Tick kMaxTicks = 60;

    constexpr Square kBlank{.row = 0, .col = 2};

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

    struct SudokuConsoleTraits final
    {
        using Summary = SudokuSummary;

        static Summary run(
            std::vector<TickEvent> script,
            const std::string &dumpPath,
            const bool loadEnabled)
        {
            return play(
                std::move(script),
                kOpenTick + 1,
                dumpPath,
                loadEnabled);
        }

        static const std::vector<std::string> &console(
            const Summary &summary)
        {
            return summary.console;
        }

        static void expectUntouched(const Summary &)
        {
        }

        static std::string scratchPrefix()
        {
            return "antwika_sudoku_console.";
        }
    };

    TEST(ConsoleIntegrationTest, Play_TypesADigitIntoTheOpenConsole)
    {
        const InputEventCodec codec;

        std::vector<TickEvent> events{
            pressAt(codec, 1, placeOf(kBlank)),
            keyAt(codec, 2, Key::Grave)};
        events.push_back(keyAt(codec, kOpenTick + 1, Key::Digit4));
        events.push_back(keyAt(codec, kOpenTick + 1, Key::Enter));

        const auto summary =
            play(std::move(events), kOpenTick + 2, "unused.json");

        EXPECT_EQ(summary.grid.at(kBlankIndex), '.');
        EXPECT_EQ(
            summary.console,
            (std::vector<std::string>{"> 4", "unknown command: 4"}));
    }

    TEST(ConsoleIntegrationTest, Play_SelectsNothingUnderTheSheet)
    {
        const InputEventCodec codec;

        std::vector<TickEvent> events{keyAt(codec, 1, Key::Grave)};
        events.push_back(pressAt(codec, kOpenTick, placeOf(kBlank)));
        events.push_back(keyAt(codec, kOpenTick + 1, Key::Grave));

        const Tick closed = kOpenTick + 1 + kConsoleAnimTicks;
        events.push_back(keyAt(codec, closed, Key::Digit4));

        const auto summary =
            play(std::move(events), closed + 1, "unused.json");

        EXPECT_EQ(summary.grid.at(kBlankIndex), '.');
        EXPECT_EQ(summary.status, Status::Playing);
    }

    TEST(ConsoleIntegrationTest, Play_RoundTripsADumpAndLoadMidGame)
    {
        const antwika::testing::ScratchFile file{
            "antwika_sudoku_console_dump_"};
        const InputEventCodec codec;

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

        std::vector<TickEvent> loading{keyAt(codec, 1, Key::Grave)};
        typeText(loading, codec, kOpenTick, "load_state");
        loading.push_back(keyAt(codec, kOpenTick, Key::Enter));

        const auto loaded = play(
            std::move(loading), kOpenTick + 1, file.string());

        EXPECT_EQ(loaded.grid.at(kBlankIndex), '4');
    }

    TEST(ConsoleIntegrationTest, Play_MountsNoConsoleWhenNoneIsAsked)
    {
        NiceMock<MockLogger> logger;
        NiceMock<MockEventSink> sink;
        const InputEventCodec codec;

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
}

namespace antwika::console::conformance
{

    INSTANTIATE_TYPED_TEST_SUITE_P(
        Sudoku, ConsoleContractTest, SudokuConsoleTraits);

    INSTANTIATE_TYPED_TEST_SUITE_P(
        Sudoku, ConsoleSnapshotRoundTripTest, SudokuConsoleTraits);

}
