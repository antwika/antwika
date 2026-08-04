#include <optional>
#include <string>

#include <gtest/gtest.h>

#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/TickEvent.hpp>

#include <antwika/sudoku/Board.hpp>
#include <antwika/sudoku/BoardFormatError.hpp>
#include <antwika/sudoku/BoardSink.hpp>
#include <antwika/sudoku/Events.hpp>
#include <antwika/sudoku/PuzzleFile.hpp>
#include <antwika/sudoku/PuzzleSource.hpp>
#include <antwika/sudoku/PuzzleState.hpp>
#include <antwika/sudoku/Status.hpp>

using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::sudoku::Board;
using antwika::sudoku::BoardFormatError;
using antwika::sudoku::BoardSink;
using antwika::sudoku::kDemoPuzzle;
using antwika::sudoku::kSolveStepBudget;
using antwika::sudoku::newPuzzleEvent;
using antwika::sudoku::PuzzleState;
using antwika::sudoku::Status;

namespace
{
    [[nodiscard]] TickEvent at(std::string name, std::string payload)
    {
        return TickEvent{
            .tick = 0,
            .event =
                Event{
                    .name = std::move(name),
                    .payload = std::move(payload)}};
    }

    [[nodiscard]] TickEvent newPuzzle()
    {
        return TickEvent{
            .tick = 0,
            .event = newPuzzleEvent(Board::parse(kDemoPuzzle))};
    }

    TEST(BoardSinkTest, Handle_StartsASessionOnTheGridItIsGiven)
    {
        PuzzleState state;
        BoardSink sink(state, kSolveStepBudget);

        sink.handle(newPuzzle());

        EXPECT_EQ(state.board().format(), kDemoPuzzle);
    }

    TEST(BoardSinkTest, Handle_WritesOneDigitIntoOneSquare)
    {
        PuzzleState state;
        BoardSink sink(state, kSolveStepBudget);
        sink.handle(newPuzzle());

        sink.handle(
            at(antwika::sudoku::events::kSetCell,
               "{\"x\":2,\"y\":0,\"digit\":4}"));

        EXPECT_EQ(state.board().at(0, 2), std::optional{4});

        sink.handle(
            at(antwika::sudoku::events::kSetCell,
               "{\"x\":2,\"y\":0,\"digit\":0}"));

        EXPECT_FALSE(state.board().at(0, 2).has_value());
    }

    TEST(BoardSinkTest, Handle_LeavesAClueAloneAndSaysSo)
    {
        PuzzleState state;
        BoardSink sink(state, kSolveStepBudget);
        sink.handle(newPuzzle());

        sink.handle(
            at(antwika::sudoku::events::kSetCell,
               "{\"x\":0,\"y\":0,\"digit\":9}"));

        EXPECT_EQ(state.board().at(0, 0), std::optional{5});
        EXPECT_EQ(state.status(), Status::GivenLocked);
    }

    TEST(BoardSinkTest, Handle_FinishesTheGridWhenAskedTo)
    {
        PuzzleState state;
        BoardSink sink(state, kSolveStepBudget);
        sink.handle(newPuzzle());

        sink.handle(at(antwika::sudoku::events::kSolve, ""));

        EXPECT_EQ(state.status(), Status::Solved);
        EXPECT_EQ(state.filled(), 81U);
    }

    TEST(BoardSinkTest, Handle_IgnoresEverythingElse)
    {
        PuzzleState state;
        BoardSink sink(state, kSolveStepBudget);
        sink.handle(newPuzzle());

        sink.handle(
            at(std::string(antwika::engine::events::kTick), ""));
        sink.handle(at("something.else", ""));

        EXPECT_EQ(state.board().format(), kDemoPuzzle);
        EXPECT_EQ(state.status(), Status::Playing);
    }

    TEST(BoardSinkTest, Handle_RefusesAPayloadOfTheWrongShape)
    {
        PuzzleState state;
        BoardSink sink(state, kSolveStepBudget);

        EXPECT_THROW(
            sink.handle(
                at(antwika::sudoku::events::kNewPuzzle, "not json")),
            BoardFormatError);
        EXPECT_THROW(
            sink.handle(
                at(antwika::sudoku::events::kNewPuzzle, "{}")),
            BoardFormatError);
        EXPECT_THROW(
            sink.handle(
                at(antwika::sudoku::events::kSetCell,
                   "{\"x\":9,\"y\":0,\"digit\":1}")),
            BoardFormatError);
    }

    TEST(BoardSinkTest, Handle_RefusesAGridThatIsNotOne)
    {
        PuzzleState state;
        BoardSink sink(state, kSolveStepBudget);

        EXPECT_THROW(
            sink.handle(
                at(antwika::sudoku::events::kNewPuzzle,
                   "{\"cells\":\"too short\"}")),
            BoardFormatError);
    }
} // namespace
