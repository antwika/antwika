#include <optional>
#include <string>

#include <gtest/gtest.h>

#include <antwika/sudoku/Board.hpp>
#include <antwika/sudoku/BoardFormatError.hpp>
#include <antwika/sudoku/PuzzleFile.hpp>
#include <antwika/sudoku/PuzzleState.hpp>
#include <antwika/sudoku/Status.hpp>

using antwika::sudoku::Board;
using antwika::sudoku::BoardFormatError;
using antwika::sudoku::kDemoPuzzle;
using antwika::sudoku::PuzzleState;
using antwika::sudoku::Square;
using antwika::sudoku::Status;

namespace
{
    // The demo puzzle's first square is a clue and its third is blank.
    constexpr Square kClue{.row = 0, .col = 0};
    constexpr Square kBlank{.row = 0, .col = 2};

    [[nodiscard]] PuzzleState started()
    {
        PuzzleState state;
        state.start(Board::parse(kDemoPuzzle));
        return state;
    }

    TEST(PuzzleStateTest, Start_TakesTheGridAsGivenAndSaysNothingYet)
    {
        const auto state = started();

        EXPECT_EQ(state.board().format(), kDemoPuzzle);
        EXPECT_EQ(state.status(), Status::Playing);
        EXPECT_FALSE(state.selected().has_value());
        EXPECT_EQ(state.filled(), 30U);
    }

    TEST(PuzzleStateTest, Start_DiscardsWhateverCameBefore)
    {
        auto state = started();
        state.select(kBlank);
        state.enter(4);

        state.start(Board{});

        EXPECT_EQ(state.board().format(), std::string(81, '.'));
        EXPECT_FALSE(state.selected().has_value());
        EXPECT_FALSE(state.isGiven(kClue));
    }

    TEST(PuzzleStateTest, IsGiven_TellsAClueFromASquareSomebodyFilled)
    {
        auto state = started();

        EXPECT_TRUE(state.isGiven(kClue));
        EXPECT_FALSE(state.isGiven(kBlank));

        state.write(kBlank, 4);

        EXPECT_FALSE(state.isGiven(kBlank));
    }

    TEST(PuzzleStateTest, Select_PicksASquareAndSaysWhenItIsAClue)
    {
        auto state = started();

        state.select(kBlank);
        EXPECT_EQ(state.selected(), (std::optional{kBlank}));
        EXPECT_EQ(state.status(), Status::Playing);

        state.select(kClue);
        EXPECT_EQ(state.selected(), (std::optional{kClue}));
        EXPECT_EQ(state.status(), Status::GivenLocked);
    }

    TEST(PuzzleStateTest, Select_RefusesASquareOutsideTheGrid)
    {
        auto state = started();

        EXPECT_THROW(
            state.select(Square{.row = 9, .col = 0}), BoardFormatError);
    }

    TEST(PuzzleStateTest, Enter_DoesNothingWhileNoSquareIsPicked)
    {
        auto state = started();

        state.enter(4);

        EXPECT_EQ(state.board().format(), kDemoPuzzle);
    }

    TEST(PuzzleStateTest, Enter_WritesIntoThePickedSquare)
    {
        auto state = started();
        state.select(kBlank);

        state.enter(4);
        EXPECT_EQ(state.board().at(0, 2), std::optional{4});

        state.enter(0);
        EXPECT_FALSE(state.board().at(0, 2).has_value());
    }

    TEST(PuzzleStateTest, Write_LeavesAClueAloneAndSaysSo)
    {
        auto state = started();

        state.write(kClue, 9);

        EXPECT_EQ(state.board().at(0, 0), std::optional{5});
        EXPECT_EQ(state.status(), Status::GivenLocked);
    }

    TEST(PuzzleStateTest, Write_NoticesAGridThatIsFinished)
    {
        auto state = started();
        state.solve();

        const Board solved = state.board();

        // Back to one square short, then finished again by hand.
        PuzzleState fresh;
        Board oneShort = solved;
        oneShort.set(8, 8, 0);
        fresh.start(oneShort);

        EXPECT_EQ(fresh.status(), Status::Playing);

        fresh.select(Square{.row = 8, .col = 8});
        fresh.enter(solved.at(8, 8).value());

        EXPECT_EQ(fresh.status(), Status::Complete);
    }

    TEST(PuzzleStateTest, Write_SaysNothingAboutAGridThatIsStillWrong)
    {
        auto state = started();
        state.select(kBlank);

        // There is a 5 in the top-left box, so this breaks a rule.
        // An unfinished grid is not something to report either way.
        state.enter(5);

        EXPECT_EQ(state.status(), Status::Playing);
    }

    TEST(PuzzleStateTest, Solve_FinishesTheGridAndKeepsTheClues)
    {
        auto state = started();

        state.solve();

        EXPECT_EQ(state.status(), Status::Solved);
        EXPECT_EQ(state.filled(), 81U);
        EXPECT_TRUE(state.isGiven(kClue));
        EXPECT_FALSE(state.isGiven(kBlank));
    }

    TEST(PuzzleStateTest, Solve_SaysSoWhenNothingCanFollowFromHere)
    {
        auto state = started();
        state.select(kBlank);

        // A 5 in the top-left box, which already has one.
        state.enter(5);
        state.solve();

        EXPECT_EQ(state.status(), Status::Unsolvable);
        EXPECT_EQ(state.board().at(0, 2), std::optional{5});
        EXPECT_EQ(state.filled(), 31U);
    }

    TEST(PuzzleStateTest, Solve_SaysSoWhenItRunsOutOfSteps)
    {
        PuzzleState state;
        state.start(Board{});

        state.solve({.maxSteps = 1});

        EXPECT_EQ(state.status(), Status::LimitExceeded);
        EXPECT_EQ(state.filled(), 0U);
    }
} // namespace
