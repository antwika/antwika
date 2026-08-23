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
using antwika::editor::getLoadBoard;
using antwika::editor::readBoard;
using antwika::editor::saveBoard;
using antwika::editor::writeBoard;
using antwika::testing::ScratchFile;

namespace
{

    [[nodiscard]] Board getWrittenBoard()
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

    [[nodiscard]] Board getRoundTripped(const Board &board)
    {
        std::stringstream stream;

        writeBoard(stream, board);

        return readBoard(stream);
    }

}

TEST(PlanFileTest, ReadBoard_TakesBackWhateverWriteBoardPutDown)
{
    const auto board = getWrittenBoard();

    EXPECT_EQ(getRoundTripped(board), board);
}

TEST(PlanFileTest, ReadBoard_TakesBackABoardHoldingNoCardsAtAll)
{
    EXPECT_EQ(getRoundTripped(Board{}), Board{});
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

    EXPECT_EQ(getRoundTripped(board), board);
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
    const auto board = getWrittenBoard();

    saveBoard(keptFile.getString(), board);

    EXPECT_EQ(getLoadBoard(keptFile.getString()), std::optional{board});
}

TEST(PlanFileTest, SaveBoard_KeepsWhatStoodInThePlaceItWritesTo)
{
    const ScratchFile keptFile("plan.");

    keptFile.write("nothing worth keeping");
    saveBoard(keptFile.getString(), getWrittenBoard());

    EXPECT_TRUE(
        std::filesystem::exists(keptFile.getString() + ".bak1"));

    std::error_code errorCode;
    std::filesystem::remove(keptFile.getString() + ".bak1", errorCode);
}

TEST(PlanFileTest, LoadBoard_FindsNoBoardWhereNoFileStands)
{
    const ScratchFile absentFile("plan.");

    EXPECT_FALSE(getLoadBoard(absentFile.getString()).has_value());
}

TEST(PlanFileTest, LoadBoard_RefusesAFileThatIsNotABoard)
{
    const ScratchFile keptFile("plan.");

    keptFile.write("{]");

    EXPECT_THROW(
        {
            [[maybe_unused]] const auto board =
                getLoadBoard(keptFile.getString());
        },
        PlanFileError);
}

TEST(PlanFileTest, SaveBoard_RefusesAPlaceItCannotWriteTo)
{
    EXPECT_THROW(
        saveBoard("no/such/folder/plan.json", Board{}),
        PlanFileError);
}
