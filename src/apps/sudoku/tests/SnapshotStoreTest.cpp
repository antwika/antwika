#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <optional>
#include <string>
#include <vector>

#include <antwika/console/SnapshotError.hpp>
#include <antwika/testing/ScratchPath.hpp>
#include <antwika/sudoku/Board.hpp>
#include <antwika/sudoku/PuzzleFile.hpp>
#include <antwika/sudoku/PuzzleState.hpp>
#include <antwika/sudoku/SnapshotStore.hpp>
#include <antwika/sudoku/Status.hpp>

using antwika::console::SnapshotError;
using antwika::sudoku::Board;
using antwika::sudoku::kDemoPuzzle;
using antwika::sudoku::kStatusCount;
using antwika::sudoku::PuzzleState;
using antwika::sudoku::puzzleStateFromJson;
using antwika::sudoku::puzzleStateToJson;
using antwika::sudoku::Square;
using antwika::sudoku::Status;
using antwika::sudoku::statusFromName;
using antwika::sudoku::statusName;
using antwika::sudoku::SudokuSnapshotStore;

namespace
{
    constexpr Square kBlank{.row = 0, .col = 2};

    [[nodiscard]] PuzzleState demoState()
    {
        PuzzleState state;
        state.start(Board::parse(kDemoPuzzle));
        return state;
    }

    [[nodiscard]] PuzzleState midGame()
    {
        PuzzleState state = demoState();
        state.select(kBlank);
        state.enter(4);
        return state;
    }

    TEST(SnapshotStoreTest, RoundTrip_KeepsAPickedSquare)
    {
        const PuzzleState dumped = midGame();

        PuzzleState restored;
        puzzleStateFromJson(puzzleStateToJson(dumped), restored);

        EXPECT_EQ(restored.board().format(), dumped.board().format());
        EXPECT_EQ(restored.clues().format(), dumped.clues().format());
        EXPECT_EQ(restored.selected(), std::optional<Square>{kBlank});
        EXPECT_EQ(restored.status(), dumped.status());
    }

    TEST(SnapshotStoreTest, RoundTrip_KeepsAnAbsentSelection)
    {
        const PuzzleState dumped = demoState();

        PuzzleState restored = midGame();
        puzzleStateFromJson(puzzleStateToJson(dumped), restored);

        EXPECT_EQ(restored.board().format(), dumped.board().format());
        EXPECT_EQ(restored.selected(), std::nullopt);
        EXPECT_EQ(restored.status(), Status::Playing);
    }

    TEST(SnapshotStoreTest, RoundTrip_KeepsEveryStatusByName)
    {
        for (std::size_t index = 0; index < kStatusCount; ++index)
        {
            const auto status = static_cast<Status>(index);

            PuzzleState dumped = demoState();
            dumped.restore(
                dumped.clues(),
                dumped.board(),
                std::nullopt,
                status);

            PuzzleState restored;
            puzzleStateFromJson(puzzleStateToJson(dumped), restored);

            EXPECT_EQ(restored.status(), status) << statusName(status);
            EXPECT_EQ(statusFromName(statusName(status)), status);
        }
    }

    TEST(SnapshotStoreTest, StatusFromName_RefusesAnUnknownName)
    {
        EXPECT_EQ(statusFromName("winning"), std::nullopt);
    }

    TEST(SnapshotStoreTest, FromJson_RefusesABadGivensString)
    {
        auto document = puzzleStateToJson(demoState());
        document["givens"] = "not a board";

        PuzzleState restored;
        EXPECT_THROW(
            puzzleStateFromJson(document, restored), SnapshotError);
    }

    TEST(SnapshotStoreTest, FromJson_RefusesABadCellsString)
    {
        auto document = puzzleStateToJson(demoState());
        document["cells"] = std::string(Board::kCellCount, 'x');

        PuzzleState restored;
        EXPECT_THROW(
            puzzleStateFromJson(document, restored), SnapshotError);
    }

    TEST(SnapshotStoreTest, FromJson_RefusesAnUnknownStatusName)
    {
        auto document = puzzleStateToJson(demoState());
        document["note"] = "winning";

        PuzzleState restored;
        EXPECT_THROW(
            puzzleStateFromJson(document, restored), SnapshotError);
    }

    TEST(SnapshotStoreTest, FromJson_RefusesAMissingMember)
    {
        auto document = puzzleStateToJson(demoState());
        document.erase("cells");

        PuzzleState restored;
        EXPECT_THROW(
            puzzleStateFromJson(document, restored), SnapshotError);
    }

    TEST(SnapshotStoreTest, FromJson_RefusesASquareOutsideTheGrid)
    {
        auto document = puzzleStateToJson(midGame());
        document["chosen"]["row"] = 9;

        PuzzleState restored;
        EXPECT_THROW(
            puzzleStateFromJson(document, restored), SnapshotError);
    }

    TEST(SnapshotStoreTest, DumpAndLoad_RoundTripThroughAFile)
    {
        const antwika::testing::ScratchFile file{
            "antwika_sudoku_snapshot_"};

        const std::vector<std::string> history{
            "> dump_state", "dumped state to somewhere"};

        PuzzleState dumped = midGame();
        SudokuSnapshotStore out(dumped);
        out.dump(file.string(), history);

        PuzzleState restored;
        SudokuSnapshotStore in(restored);
        EXPECT_EQ(in.load(file.string()), history);

        EXPECT_EQ(restored.board().format(), dumped.board().format());
        EXPECT_EQ(restored.selected(), std::optional<Square>{kBlank});
    }

    TEST(SnapshotStoreTest, Load_RefusesAMissingFile)
    {
        PuzzleState state = demoState();
        SudokuSnapshotStore store(state);

        EXPECT_THROW(
            static_cast<void>(store.load(
                antwika::testing::scratchPath(
                    "antwika_sudoku_snapshot_missing_")
                    .string())),
            SnapshotError);
    }
}
