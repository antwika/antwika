#include <gtest/gtest.h>

#include <filesystem>
#include <string>
#include <vector>

#include <antwika/testing/ScratchPath.hpp>
#include <antwika/testing/ScratchDirectory.hpp>

#include "antwika/io/FileList.hpp"

using antwika::io::entriesIn;
using antwika::io::getEntryText;
using antwika::io::FileEntry;
using antwika::io::kParentEntry;
using antwika::io::pathIn;
using antwika::testing::ScratchDirectory;

namespace
{
    [[nodiscard]] FileEntry getParent()
    {
        return FileEntry{
            .name = std::string{kParentEntry}, .directory = true};
    }
}

TEST(FileListTest, EntriesIn_NamesEveryFileInReadingOrder)
{
    const ScratchDirectory scratchDirectory("io_files");

    scratchDirectory.write("second.png", "x");
    scratchDirectory.write("first.png", "x");

    EXPECT_EQ(
        entriesIn(scratchDirectory.getString()),
        (std::vector<FileEntry>{
            getParent(),
            FileEntry{.name = "first.png"},
            FileEntry{.name = "second.png"}}));
}

TEST(FileListTest, EntriesIn_PutsTheDirectoriesAheadOfTheFiles)
{
    const ScratchDirectory scratchDirectory("io_folders");
    std::filesystem::create_directories(scratchDirectory.pathIn("nested"));

    scratchDirectory.write("alone.png", "x");

    EXPECT_EQ(
        entriesIn(scratchDirectory.getString()),
        (std::vector<FileEntry>{
            getParent(),
            FileEntry{.name = "nested", .directory = true},
            FileEntry{.name = "alone.png"}}));
}

TEST(FileListTest, EntriesIn_LeadsWithTheParentEvenWhenItCannotRead)
{
    const ScratchDirectory scratchDirectory("io_absent");

    EXPECT_EQ(
        entriesIn(scratchDirectory.pathIn("nowhere")),
        (std::vector<FileEntry>{getParent()}));
}

TEST(FileListTest, EntriesIn_SortsTheDirectoriesAmongThemselves)
{
    const ScratchDirectory scratchDirectory("io_sorted");
    std::filesystem::create_directories(scratchDirectory.pathIn("beta"));
    std::filesystem::create_directories(scratchDirectory.pathIn("alpha"));

    EXPECT_EQ(
        entriesIn(scratchDirectory.getString()),
        (std::vector<FileEntry>{
            getParent(),
            FileEntry{.name = "alpha", .directory = true},
            FileEntry{.name = "beta", .directory = true}}));
}

TEST(FileListTest, PathIn_JoinsANameOntoItsDirectory)
{
    EXPECT_EQ(pathIn("sprites", "atlas.png"), "sprites/atlas.png");
}

TEST(FileListTest, PathIn_FoldsAParentStepAway)
{
    EXPECT_EQ(pathIn("sprites/tiles", ".."), "sprites");
    EXPECT_EQ(pathIn(".", ".."), "..");
}

TEST(FileListTest, PathIn_KeepsANameAloneOutOfTheWorkingDirectory)
{
    EXPECT_EQ(pathIn(".", "atlas.png"), "atlas.png");
}

TEST(FileListTest, EntryText_MarksADirectoryWithATrailingSlash)
{
    EXPECT_EQ(
        getEntryText(FileEntry{.name = "sprites", .directory = true}),
        "sprites/");
    EXPECT_EQ(getEntryText(getParent()), "../");
}

TEST(FileListTest, EntryText_LeavesAFileNameAlone)
{
    EXPECT_EQ(getEntryText(FileEntry{.name = "atlas.png"}), "atlas.png");
}

TEST(FileListTest, FileEntry_ComparesEveryFieldItCarries)
{
    const FileEntry namedEntry{.name = "atlas.png", .directory = false};

    EXPECT_EQ(namedEntry, (FileEntry{.name = "atlas.png"}));
    EXPECT_NE(namedEntry, (FileEntry{.name = "other.png"}));
    EXPECT_NE(
        namedEntry, (FileEntry{.name = "atlas.png", .directory = true}));
}

TEST(FileListTest, EntriesIn_LeavesOutWhatIsNeitherFileNorDirectory)
{
    const ScratchDirectory scratchDirectory("io_odd");

    std::filesystem::create_symlink(
        scratchDirectory.pathIn(
            "nothing-here"), scratchDirectory.pathIn("dangling"));

    EXPECT_EQ(
        entriesIn(scratchDirectory.getString()),
        (std::vector<FileEntry>{getParent()}));
}

TEST(FileListTest, EntriesIn_ListsPastAnEntryItCannotRead)
{
    const ScratchDirectory scratchDirectory("io_unreadable");

    std::filesystem::create_symlink(
        scratchDirectory.pathIn(
            "nothing-here"), scratchDirectory.pathIn("dangling"));
    std::filesystem::create_directories(scratchDirectory.pathIn("nested"));

    scratchDirectory.write("alone.png", "x");

    EXPECT_EQ(
        entriesIn(scratchDirectory.getString()),
        (std::vector<FileEntry>{
            getParent(),
            FileEntry{.name = "nested", .directory = true},
            FileEntry{.name = "alone.png"}}));
}

TEST(FileListTest, EntriesIn_NamesAFileTooLongToSitInsideAString)
{
    const ScratchDirectory scratchDirectory("io_long");
    const std::string longName =
        "a-sprite-sheet-with-a-very-long-name-indeed.png";

    scratchDirectory.write(longName, "x");

    EXPECT_EQ(
        entriesIn(scratchDirectory.getString()),
        (std::vector<FileEntry>{getParent(), FileEntry{.name = longName}}));
}

TEST(FileListTest, EntriesIn_NamesADirectoryTooLongToSitInsideAString)
{
    const ScratchDirectory scratchDirectory("io_longdir");
    const std::string longName =
        "a-directory-with-a-very-long-name-indeed-here";

    std::filesystem::create_directories(scratchDirectory.pathIn(longName));

    EXPECT_EQ(
        entriesIn(scratchDirectory.getString()),
        (std::vector<FileEntry>{
            getParent(), FileEntry{.name = longName, .directory = true}}));
}

TEST(FileListTest, PathIn_JoinsANameTooLongToSitInsideAString)
{
    EXPECT_EQ(
        pathIn(
            "a-directory-with-a-very-long-name-indeed-here",
            "a-sprite-sheet-with-a-very-long-name-indeed.png"),
        "a-directory-with-a-very-long-name-indeed-here/"
        "a-sprite-sheet-with-a-very-long-name-indeed.png");
}
