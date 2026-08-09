#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <unistd.h>

#include <antwika/testing/ScratchPath.hpp>

#include "antwika/music_editor/ScoreFiles.hpp"

using antwika::music_editor::listScores;
using antwika::music_editor::loadScore;
using antwika::music_editor::safeScoreName;
using antwika::music_editor::saveScore;
using antwika::music_editor::ScoreFileError;
using antwika::music_editor::scorePath;

namespace
{
    [[nodiscard]] std::string freshDirectory(const std::string &name)
    {
        const auto path =
            std::filesystem::temp_directory_path()
            / ("antwika-scores." + std::to_string(::getpid()))
            / name;

        std::filesystem::remove_all(path);

        return path.string();
    }
}

TEST(ScoreFilesTest, ScorePath_PathIsDirectoryNameAndExtension)
{
    EXPECT_EQ(scorePath("scores", "beat"), "scores/beat.score");
}

TEST(ScoreFilesTest, SafeScoreName_KeepsLettersDigitsAndDashes)
{
    EXPECT_EQ(safeScoreName("my-Beat_2"), "my-Beat_2");
    EXPECT_EQ(safeScoreName("../etc/passwd"), "etcpasswd");
    EXPECT_EQ(safeScoreName("a b.score"), "abscore");
    EXPECT_EQ(safeScoreName("..//"), "");

    EXPECT_EQ(safeScoreName("`az{"), "az");
    EXPECT_EQ(safeScoreName("@AZ["), "AZ");
    EXPECT_EQ(safeScoreName("/09:"), "09");
}

TEST(ScoreFilesTest, LoadScore_SavesAndLoadsTheSameDocumentBack)
{
    const auto directory = freshDirectory("roundtrip");
    const auto path = scorePath(directory, "beat");

    saveScore(path, "$: drum.n(\"0\")\n");

    EXPECT_EQ(loadScore(path), "$: drum.n(\"0\")\n");
}

TEST(ScoreFilesTest, ListScores_SavingMakesTheDirectoryItself)
{
    const auto directory = freshDirectory("fresh") + "/deeper";

    saveScore(scorePath(directory, "beat"), "x");

    EXPECT_EQ(listScores(directory).size(), 1U);
}

TEST(ScoreFilesTest, ListScores_ListsSortedNamesWithoutExtensions)
{
    const auto directory = freshDirectory("listing");

    saveScore(scorePath(directory, "zed"), "z");
    saveScore(scorePath(directory, "alpha"), "a");

    std::ofstream(directory + "/notes.txt") << "not a score";
    std::ofstream(directory + "/.score") << "nameless";

    const auto names = listScores(directory);

    ASSERT_EQ(names.size(), 2U);
    EXPECT_EQ(names[0], "alpha");
    EXPECT_EQ(names[1], "zed");
}

TEST(ScoreFilesTest, ListScores_ReadsAMissingDirectoryAsNone)
{
    EXPECT_TRUE(listScores(freshDirectory("missing")).empty());
}

TEST(ScoreFilesTest, SaveScore_RefusesWhereNoDirectoryCanBe)
{
    const auto directory = freshDirectory("blocked");

    std::filesystem::create_directories(
        std::filesystem::path(directory).parent_path());
    std::ofstream(directory) << "in the way";

    EXPECT_THROW(
        saveScore(scorePath(directory, "beat"), "x"), ScoreFileError);
}

TEST(ScoreFilesTest, SaveScore_RefusesAFileThatWillNotOpen)
{
    const auto directory = freshDirectory("unwritable");

    std::filesystem::create_directories(
        scorePath(directory, "beat"));

    EXPECT_THROW(
        saveScore(scorePath(directory, "beat"), "x"), ScoreFileError);
}

TEST(ScoreFilesTest, LoadScore_RefusesToLoadWhatIsNotThere)
{
    const auto directory = freshDirectory("empty");

    EXPECT_THROW(
        (void)loadScore(scorePath(directory, "ghost")),
        ScoreFileError);
}

TEST(ScoreFilesTest, LoadScore_AnEmptyScoreLoadsBackEmpty)
{
    const auto directory = freshDirectory("blank");
    const auto path = scorePath(directory, "nothing");

    saveScore(path, "");

    EXPECT_EQ(loadScore(path), "");
}
