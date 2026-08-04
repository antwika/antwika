#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/mocks/MockEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/event/TickEventRecorder.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/i18n/Locale.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/replay/ReplayCli.hpp>
#include <antwika/replay/ReplaySource.hpp>

#include <antwika/sudoku/Board.hpp>
#include <antwika/sudoku/BoardOverlay.hpp>
#include <antwika/sudoku/PuzzleFile.hpp>
#include <antwika/sudoku/PuzzleSource.hpp>
#include <antwika/sudoku/PuzzleState.hpp>
#include <antwika/sudoku/Status.hpp>
#include <antwika/sudoku/Sudoku.hpp>
#include <antwika/sudoku/SudokuScene.hpp>
#include <antwika/app/TickLimitSource.hpp>
#include <antwika/sudoku/Widgets.hpp>
#include <antwika/testing/ScratchPath.hpp>

#include "antwika/sudoku/Messages.hpp"
#include "WidgetCentre.hpp"

using antwika::event::Event;
using antwika::event::ITickEventSink;
using antwika::event::mocks::MockEventSink;
using antwika::event::TickEvent;
using antwika::event::TickEventRecorder;
using antwika::gfx::Point;
using antwika::gfx::Size;
using antwika::input::InputEventCodec;
using antwika::input::Key;
using antwika::input::KeyPressed;
using antwika::input::MouseButton;
using antwika::input::PointerButtonPressed;
using antwika::log::mocks::MockLogger;
using antwika::replay::ReplaySource;
using antwika::sudoku::Board;
using antwika::sudoku::BoardOverlay;
using antwika::sudoku::kDemoPuzzle;
using antwika::sudoku::PuzzleSource;
using antwika::sudoku::PuzzleState;
using antwika::sudoku::Square;
using antwika::sudoku::Status;
using antwika::sudoku::SudokuScene;
using antwika::sudoku::SudokuSummary;
using antwika::app::TickLimitSource;
using antwika::sudoku::tests::squareCentre;
using antwika::sudoku::tests::widgetCentre;
using ::testing::NiceMock;
namespace widgets = antwika::sudoku::widgets;

namespace
{
    constexpr antwika::sudoku::Translator kTranslator{
        antwika::i18n::kDefaultLocale};

    constexpr Size kCanvas{.width = 720, .height = 800};
    constexpr antwika::time::Tick kStopTick = 6;
    constexpr antwika::time::Tick kMaxTicks = 30;

    constexpr Square kBlank{.row = 0, .col = 2};

    // What the demo puzzle's one blank square takes in its solution.
    constexpr int kBlankAnswer = 4;

    [[nodiscard]] PuzzleState demoState()
    {
        PuzzleState state;
        state.start(Board::parse(kDemoPuzzle));
        return state;
    }

    [[nodiscard]] Point placeOf(const Square square)
    {
        const SudokuScene scene{kTranslator};
        const auto found = squareCentre(
            scene.describe(kCanvas, {}, demoState()), square);
        EXPECT_TRUE(found.has_value());
        return found.value_or(Point{});
    }

    [[nodiscard]] Point solveButton()
    {
        const SudokuScene scene{kTranslator};
        const auto found = widgetCentre(
            scene.describe(kCanvas, {}, demoState()), widgets::kSolve);
        EXPECT_TRUE(found.has_value());
        return found.value_or(Point{});
    }

    // Pick the first blank square, type a 4 in it, then press Solve.
    // Every position is read off the layout rather than guessed at.
    // Which is the same thing ui::Frame::rects does for the app.
    [[nodiscard]] std::vector<TickEvent> script()
    {
        const InputEventCodec codec;

        return {
            TickEvent{
                .tick = 1,
                .event = codec.encode(PointerButtonPressed{
                    .button = MouseButton::Left,
                    .position =
                        {.x = placeOf(kBlank).x,
                         .y = placeOf(kBlank).y}})},
            TickEvent{
                .tick = 2,
                .event = codec.encode(KeyPressed{
                    .key = Key::Digit4, .modifiers = {}})},
            TickEvent{
                .tick = 3,
                .event = codec.encode(PointerButtonPressed{
                    .button = MouseButton::Left,
                    .position =
                        {.x = solveButton().x,
                         .y = solveButton().y}})}};
    }

    /**
     * @brief Run one session over a script, and report what it ended
     * on.
     *
     * The one call a live run and a replayed one differ only inside:
     * the puzzle is announced by PuzzleSource when there is one, and
     * carried by the recording when there is not.
     *
     * @param events The events the session is driven by.
     * @param puzzle The grid to announce, or nothing on a replay.
     * @param recorder Where every dispatched event is kept, if
     * anywhere.
     * @return What the session ended on.
     */
    [[nodiscard]] SudokuSummary play(
        std::vector<TickEvent> events,
        std::optional<Board> puzzle,
        TickEventRecorder *recorder = nullptr)
    {
        NiceMock<MockLogger> logger;
        NiceMock<MockEventSink> sink;
        const InputEventCodec codec;

        ReplaySource file(std::move(events));
        PuzzleSource puzzled(file, std::move(puzzle));
        TickLimitSource source(puzzled, kStopTick);

        std::optional<std::reference_wrapper<ITickEventSink>> kept;
        if (recorder != nullptr)
        {
            kept = *recorder;
        }

        return antwika::sudoku::bootstrap({
            .logger = logger,
            .eventSink = sink,
            .inputSource = source,
            .codec = codec,
            .translator = kTranslator,
            .canvas = kCanvas,
            .maxTicks = kMaxTicks,
            .replayRecorder = kept});
    }

    TEST(ReplayIntegrationTest, Bootstrap_PlaysAClickAKeyAndASolve)
    {
        const auto summary =
            play(script(), Board::parse(kDemoPuzzle));

        EXPECT_EQ(summary.status, Status::Solved);
        EXPECT_EQ(summary.filled, 81U);
        EXPECT_EQ(summary.grid.at(2), '0' + kBlankAnswer);
        EXPECT_GT(summary.commands, 0U);
    }

    /**
     * @brief Write a session's events out and read them back.
     *
     * Through the real saveReplayFile()/loadReplayFile() pair rather
     * than straight off the recorder, because the filtering those do --
     * engine.tick above all -- is what `--record` relies on, and a
     * round trip that skipped them would be proving something no
     * `--replay` run ever does.
     *
     * @param recorded What the run dispatched.
     * @return What a `--replay` of it would be handed.
     */
    [[nodiscard]] std::vector<TickEvent> throughAFile(
        std::vector<TickEvent> recorded)
    {
        // Named after the case that is running.
        // antwika_bundle_test registers every case with CTest.
        // So two cases calling this are two parallel processes.
        // One fixed name is one file they would fight over.
        const antwika::testing::ScratchFile file{
            std::string{"antwika_sudoku_replay_"}
            + ::testing::UnitTest::GetInstance()->current_test_info()
                  ->name()
            + ".json"};
        antwika::replay::saveReplayFile(std::move(recorded), file.string());

        return antwika::replay::loadReplayFile(file.string());
    }

    TEST(ReplayIntegrationTest, Bootstrap_RecordsOnlyWhatCameFromOutside)
    {
        TickEventRecorder recorder;
        const auto live = play(
            script(), Board::parse(kDemoPuzzle), &recorder);

        EXPECT_EQ(live.status, Status::Solved);

        const auto saved = throughAFile(recorder.getEvents());

        EXPECT_FALSE(saved.empty());

        std::size_t announcements = 0;

        for (const auto &event : saved)
        {
            const auto &name = event.event.name;

            // Never a ui.* name.
            // And never a digit or a grid this session worked out.
            EXPECT_NE(name.rfind("ui.", 0), 0U);
            EXPECT_NE(name, "sudoku.set_cell");
            EXPECT_NE(name, "sudoku.solve");
            EXPECT_NE(name, antwika::engine::events::kTick);

            if (name == "sudoku.new_puzzle")
            {
                ++announcements;
            }
        }

        // The puzzle came from outside, so it is recorded once.
        EXPECT_EQ(announcements, 1U);
    }

    TEST(ReplayIntegrationTest, Bootstrap_ReplaysToTheSameGrid)
    {
        TickEventRecorder recorder;
        const auto live = play(
            script(), Board::parse(kDemoPuzzle), &recorder);

        // Nothing is announced this time.
        // The recording carries the grid it was played on.
        // Which is the whole reason the puzzle travels as an event.
        const auto replayed =
            play(throughAFile(recorder.getEvents()), std::nullopt);

        EXPECT_EQ(replayed.grid, live.grid);
        EXPECT_EQ(replayed.filled, live.filled);
        EXPECT_EQ(replayed.status, live.status);
    }

    TEST(ReplayIntegrationTest, Bootstrap_RunsWithNothingHappeningAtAll)
    {
        const auto summary = play({}, std::nullopt);

        EXPECT_EQ(summary.status, Status::Playing);
        EXPECT_EQ(summary.filled, 0U);
    }

    /**
     * @brief Stands in for the renderer main.cpp hands bootstrap().
     *
     * It reads exactly what RenderSink reads, so what it sees is what
     * a frame would have been drawn from.
     */
    class FinishedTickWatcher final : public ITickEventSink
    {
    public:
        FinishedTickWatcher(
            const PuzzleState &state,
            const BoardOverlay &overlay,
            std::vector<std::string> &seen)
            : state(state), overlay(overlay), seen(seen)
        {
        }

        void handle(const TickEvent &event) override
        {
            if (event.event.name != antwika::engine::events::kTick)
            {
                return;
            }

            EXPECT_FALSE(overlay.commands().empty());
            seen.push_back(state.board().format());
        }

    private:
        const PuzzleState &state;
        const BoardOverlay &overlay;
        std::vector<std::string> &seen;
    };

    TEST(ReplayIntegrationTest, Bootstrap_ShowsEveryTickTheGridItEndedOn)
    {
        NiceMock<MockLogger> logger;
        NiceMock<MockEventSink> sink;
        const InputEventCodec codec;

        ReplaySource file(script());
        PuzzleSource puzzled(file, Board::parse(kDemoPuzzle));
        TickLimitSource source(puzzled, kStopTick);

        std::vector<std::string> seen;

        const auto summary = antwika::sudoku::bootstrap({
            .logger = logger,
            .eventSink = sink,
            .inputSource = source,
            .codec = codec,
            .translator = kTranslator,
            .canvas = kCanvas,
            .maxTicks = kMaxTicks,
            .extraSink =
                [&seen](
                    const PuzzleState &state,
                    const BoardOverlay &overlay)
            {
                return std::make_unique<FinishedTickWatcher>(
                    state, overlay, seen);
            }});

        // The tick the stop lands on still runs to completion.
        EXPECT_EQ(seen.size(), kStopTick + 1);
        EXPECT_EQ(seen.back(), summary.grid);

        // The grid the session ends on is not the grid it started on.
        EXPECT_NE(seen.front(), summary.grid);
    }
} // namespace
