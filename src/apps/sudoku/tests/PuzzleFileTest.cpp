#include <optional>
#include <sstream>
#include <string>

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include <antwika/replay/SchemaVersion.hpp>

#include <antwika/sudoku/Board.hpp>
#include <antwika/sudoku/BoardFormatError.hpp>
#include <antwika/sudoku/PuzzleFile.hpp>

#include "ScratchFile.hpp"

using antwika::sudoku::Board;
using antwika::sudoku::BoardFormatError;
using antwika::sudoku::kDemoPuzzle;
using antwika::sudoku::kPuzzleDocumentVersion;
using antwika::sudoku::kPuzzleMagic;
using antwika::sudoku::parsePuzzleDocument;
using antwika::sudoku::puzzleFromJson;
using antwika::sudoku::readPuzzle;
using antwika::sudoku::standardPuzzleMigrations;
using antwika::sudoku::startingPuzzle;
using antwika::sudoku::tests::ScratchFile;

namespace
{
    const std::string kVersionKey{antwika::replay::kSchemaVersionKey};

    [[nodiscard]] nlohmann::json currentDocument()
    {
        nlohmann::json document;
        document["magic"] = std::string(kPuzzleMagic);
        document[kVersionKey] = kPuzzleDocumentVersion;
        document["cells"] = std::string(kDemoPuzzle);
        return document;
    }

    TEST(PuzzleFileTest, Migrations_EndAtTheVersionThisBuildWrites)
    {
        EXPECT_EQ(
            standardPuzzleMigrations().currentVersion(),
            kPuzzleDocumentVersion);
    }

    TEST(PuzzleFileTest, ParsePuzzleDocument_WrapsTheFlatGridAsIs)
    {
        const auto document = parsePuzzleDocument(kDemoPuzzle);

        EXPECT_EQ(
            document.at("cells").get<std::string>(), kDemoPuzzle);
        EXPECT_FALSE(document.contains(kVersionKey));
    }

    TEST(PuzzleFileTest, ParsePuzzleDocument_ReadsA81DigitGridAsAGrid)
    {
        // 81 digits are also a valid JSON number.
        // Which is why the shape is decided on the first character.
        // Rather than on whether a parse happens to succeed.
        const std::string filled(81, '5');
        const auto document = parsePuzzleDocument(filled);

        EXPECT_EQ(document.at("cells").get<std::string>(), filled);
    }

    TEST(PuzzleFileTest, ParsePuzzleDocument_ReadsNothingAsAGridToo)
    {
        // Whitespace and nothing else opens like no JSON at all.
        // Board::parse() is what then refuses it, and says why.
        EXPECT_EQ(
            parsePuzzleDocument("  \n ").at("cells").get<std::string>(),
            "  \n ");
    }

    TEST(PuzzleFileTest, ParsePuzzleDocument_ReadsAnObjectAsJson)
    {
        const auto document =
            parsePuzzleDocument("  \n" + currentDocument().dump());

        EXPECT_EQ(
            document.at("magic").get<std::string>(), kPuzzleMagic);
    }

    TEST(PuzzleFileTest, ParsePuzzleDocument_RefusesBrokenJson)
    {
        EXPECT_THROW(
            (void)parsePuzzleDocument("{\"cells\":"),
            BoardFormatError);
    }

    TEST(PuzzleFileTest, PuzzleFromJson_ReadsTheVersionThisBuildWrites)
    {
        EXPECT_EQ(
            puzzleFromJson(currentDocument()).format(), kDemoPuzzle);
    }

    TEST(PuzzleFileTest, PuzzleFromJson_MigratesAFlatGridUpToCurrent)
    {
        // What a document written before the version member becomes.
        // A grid, and nothing at all saying what it is.
        nlohmann::json version1;
        version1["cells"] = std::string(kDemoPuzzle);

        EXPECT_EQ(puzzleFromJson(version1).format(), kDemoPuzzle);
    }

    TEST(PuzzleFileTest, PuzzleFromJson_RefusesAVersionFromTheFuture)
    {
        auto document = currentDocument();
        document[kVersionKey] = kPuzzleDocumentVersion + 1;

        EXPECT_THROW(
            (void)puzzleFromJson(document), BoardFormatError);
    }

    TEST(PuzzleFileTest, PuzzleFromJson_RefusesAnotherFormatsDocument)
    {
        // A replay states its version in the very same member.
        // So the magic is the only thing telling the two apart.
        nlohmann::json replay;
        replay["magic"] = "antwika-replay";
        replay[kVersionKey] = 1;
        replay["events"] = nlohmann::json::array();

        EXPECT_THROW(
            (void)puzzleFromJson(replay), BoardFormatError);
    }

    TEST(PuzzleFileTest, PuzzleFromJson_RefusesAGridThatIsNotOne)
    {
        auto document = currentDocument();
        document["cells"] = "too short";

        EXPECT_THROW(
            (void)puzzleFromJson(document), BoardFormatError);
    }

    TEST(PuzzleFileTest, ReadPuzzle_ReadsBothShapesOffAStream)
    {
        std::istringstream flat{std::string(kDemoPuzzle)};
        EXPECT_EQ(readPuzzle(flat).format(), kDemoPuzzle);

        std::istringstream json{currentDocument().dump()};
        EXPECT_EQ(readPuzzle(json).format(), kDemoPuzzle);
    }

    TEST(StartingPuzzleTest, StartingPuzzle_FallsBackOnTheDemoGrid)
    {
        const auto puzzle = startingPuzzle(std::nullopt, false);

        ASSERT_TRUE(puzzle.has_value());
        EXPECT_EQ(puzzle->format(), kDemoPuzzle);
    }

    TEST(StartingPuzzleTest, StartingPuzzle_ReadsTheFileItIsGiven)
    {
        const ScratchFile file{"antwika_sudoku_puzzle.txt"};
        file.write(
            "1........\n"
            ".........\n"
            ".........\n"
            ".........\n"
            ".........\n"
            ".........\n"
            ".........\n"
            ".........\n"
            ".........\n");

        const auto puzzle = startingPuzzle(file.string(), false);

        ASSERT_TRUE(puzzle.has_value());
        EXPECT_EQ(puzzle->at(0, 0), std::optional{1});
    }

    TEST(StartingPuzzleTest, StartingPuzzle_SaysWhichFileItCouldNotOpen)
    {
        EXPECT_THROW(
            (void)startingPuzzle(
                std::optional<std::string>{"no-such-puzzle.txt"},
                false),
            BoardFormatError);
    }

    TEST(StartingPuzzleTest, StartingPuzzle_AnnouncesNothingOnAReplay)
    {
        // The recording carries its own sudoku.new_puzzle.
        // A second one would start the session on another grid.
        // Which the recorded clicks were never aimed at.
        EXPECT_FALSE(startingPuzzle(std::nullopt, true).has_value());
        EXPECT_FALSE(
            startingPuzzle(
                std::optional<std::string>{"ignored.txt"}, true)
                .has_value());
    }
} // namespace
