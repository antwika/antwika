#include <gtest/gtest.h>

#include <filesystem>
#include <optional>
#include <sstream>
#include <system_error>
#include <string>

#include <antwika/testing/ScratchPath.hpp>
#include <antwika/testing/ScratchFile.hpp>

#include "antwika/editor/plan/PlanFile.hpp"
#include "antwika/editor/plan/PlanFileError.hpp"

using antwika::editor::Board;
using antwika::editor::Card;
using antwika::editor::Column;
using antwika::editor::PlanFileError;
using antwika::editor::addCard;
using antwika::editor::cardsOf;
using antwika::editor::loadBoard;
using antwika::editor::readBoard;
using antwika::editor::saveBoard;
using antwika::editor::writeBoard;
using antwika::testing::ScratchFile;

namespace
{

    [[nodiscard]] Board writtenBoard()
    {
        Board board;

        EXPECT_TRUE(
            addCard(
                board,
                Column::Todo,
                Card{.title = "Ramp corners",
                     .body = "North-facing ramps meet wrong."}));
        EXPECT_TRUE(
            addCard(
                board,
                Column::Doing,
                Card{.title = "Water tiles", .body = ""}));
        EXPECT_TRUE(
            addCard(
                board,
                Column::Done,
                Card{.title = "Bloom", .body = ""}));

        return board;
    }

    [[nodiscard]] Board roundTripped(const Board &board)
    {
        std::stringstream stream;

        writeBoard(stream, board);

        return readBoard(stream);
    }

}

TEST(PlanFileTest, ReadBoard_TakesBackWhateverWriteBoardPutDown)
{
    const auto board = writtenBoard();

    EXPECT_EQ(roundTripped(board), board);
}

TEST(PlanFileTest, ReadBoard_TakesBackABoardHoldingNoCardsAtAll)
{
    EXPECT_EQ(roundTripped(Board{}), Board{});
}

TEST(PlanFileTest, ReadBoard_KeepsTheLinesAndMarksOfADescription)
{
    Board board;

    EXPECT_TRUE(
        addCard(
            board,
            Column::Todo,
            Card{.title = "A card: with marks?",
                 .body = "one\ntwo\n\tthree \"quoted\""}));

    EXPECT_EQ(roundTripped(board), board);
}

TEST(PlanFileTest, ReadBoard_RefusesAStreamThatIsNotJsonAtAll)
{
    std::stringstream stream("this is not a document");

    EXPECT_THROW(
        { [[maybe_unused]] const auto board = readBoard(stream); },
        PlanFileError);
}

TEST(PlanFileTest, ReadBoard_RefusesADocumentThatIsNotABoard)
{
    std::stringstream stream(R"({"magic": "antwika.map"})");

    EXPECT_THROW(
        { [[maybe_unused]] const auto board = readBoard(stream); },
        PlanFileError);
}

TEST(PlanFileTest, ReadBoard_RefusesABoardWithAColumnMissing)
{
    std::stringstream stream(
        R"({"magic": "antwika.plan", "version": 1,
            "columns": {"todo": [], "doing": []}})");

    EXPECT_THROW(
        { [[maybe_unused]] const auto board = readBoard(stream); },
        PlanFileError);
}

TEST(PlanFileTest, ReadBoard_RefusesACardWithNoDescriptionOfItsOwn)
{
    std::stringstream stream(
        R"({"magic": "antwika.plan", "version": 1,
            "columns": {"todo": [{"title": "half a card"}],
                        "doing": [], "done": []}})");

    EXPECT_THROW(
        { [[maybe_unused]] const auto board = readBoard(stream); },
        PlanFileError);
}

TEST(PlanFileTest, ReadBoard_RefusesAReadingThisBuildDoesNotKnow)
{
    std::stringstream stream(
        R"({"magic": "antwika.plan", "version": 99,
            "columns": {"todo": [], "doing": [], "done": []}})");

    EXPECT_THROW(
        { [[maybe_unused]] const auto board = readBoard(stream); },
        PlanFileError);
}

TEST(PlanFileTest, LoadBoard_TakesBackWhateverSaveBoardPutDown)
{
    const ScratchFile keptFile("plan.");
    const auto board = writtenBoard();

    saveBoard(keptFile.string(), board);

    EXPECT_EQ(loadBoard(keptFile.string()), std::optional{board});
}

TEST(PlanFileTest, SaveBoard_KeepsWhatStoodInThePlaceItWritesTo)
{
    const ScratchFile keptFile("plan.");

    keptFile.write("nothing worth keeping");
    saveBoard(keptFile.string(), writtenBoard());

    EXPECT_TRUE(
        std::filesystem::exists(keptFile.string() + ".bak1"));

    std::error_code errorCode;
    std::filesystem::remove(keptFile.string() + ".bak1", errorCode);
}

TEST(PlanFileTest, LoadBoard_FindsNoBoardWhereNoFileStands)
{
    const ScratchFile absentFile("plan.");

    EXPECT_FALSE(loadBoard(absentFile.string()).has_value());
}

TEST(PlanFileTest, LoadBoard_RefusesAFileThatIsNotABoard)
{
    const ScratchFile keptFile("plan.");

    keptFile.write("{]");

    EXPECT_THROW(
        {
            [[maybe_unused]] const auto board =
                loadBoard(keptFile.string());
        },
        PlanFileError);
}

TEST(PlanFileTest, SaveBoard_RefusesAPlaceItCannotWriteTo)
{
    EXPECT_THROW(
        saveBoard("no/such/folder/plan.json", Board{}),
        PlanFileError);
}
