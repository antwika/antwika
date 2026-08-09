#include <gtest/gtest.h>

#include <filesystem>
#include <string>
#include <vector>

#include <antwika/testing/ScratchPath.hpp>

#include "antwika/atlas_editor/FileList.hpp"

using antwika::atlas_editor::entriesIn;
using antwika::atlas_editor::entryText;
using antwika::atlas_editor::FileEntry;
using antwika::atlas_editor::kParentEntry;
using antwika::atlas_editor::pathIn;
using antwika::testing::ScratchDirectory;

namespace
{
    [[nodiscard]] FileEntry parent()
    {
        return FileEntry{
            .name = std::string{kParentEntry}, .directory = true};
    }
}

TEST(FileListTest, EntriesIn_NamesEveryFileInReadingOrder)
{
    const ScratchDirectory dir("atlas_editor_files");

    dir.write("second.png", "x");
    dir.write("first.png", "x");

    EXPECT_EQ(
        entriesIn(dir.string()),
        (std::vector<FileEntry>{
            parent(),
            FileEntry{.name = "first.png"},
            FileEntry{.name = "second.png"}}));
}

TEST(FileListTest, EntriesIn_PutsTheDirectoriesAheadOfTheFiles)
{
    const ScratchDirectory dir("atlas_editor_folders");
    std::filesystem::create_directories(dir.pathIn("nested"));

    dir.write("alone.png", "x");

    EXPECT_EQ(
        entriesIn(dir.string()),
        (std::vector<FileEntry>{
            parent(),
            FileEntry{.name = "nested", .directory = true},
            FileEntry{.name = "alone.png"}}));
}

TEST(FileListTest, EntriesIn_LeadsWithTheParentEvenWhenItCannotRead)
{
    const ScratchDirectory dir("atlas_editor_absent");

    EXPECT_EQ(
        entriesIn(dir.pathIn("nowhere")),
        (std::vector<FileEntry>{parent()}));
}

TEST(FileListTest, EntriesIn_SortsTheDirectoriesAmongThemselves)
{
    const ScratchDirectory dir("atlas_editor_sorted");
    std::filesystem::create_directories(dir.pathIn("beta"));
    std::filesystem::create_directories(dir.pathIn("alpha"));

    EXPECT_EQ(
        entriesIn(dir.string()),
        (std::vector<FileEntry>{
            parent(),
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
        entryText(FileEntry{.name = "sprites", .directory = true}),
        "sprites/");
    EXPECT_EQ(entryText(parent()), "../");
}

TEST(FileListTest, EntryText_LeavesAFileNameAlone)
{
    EXPECT_EQ(entryText(FileEntry{.name = "atlas.png"}), "atlas.png");
}

TEST(FileListTest, FileEntry_ComparesEveryFieldItCarries)
{
    const FileEntry named{.name = "atlas.png", .directory = false};

    EXPECT_EQ(named, (FileEntry{.name = "atlas.png"}));
    EXPECT_NE(named, (FileEntry{.name = "other.png"}));
    EXPECT_NE(
        named, (FileEntry{.name = "atlas.png", .directory = true}));
}

TEST(FileListTest, EntriesIn_LeavesOutWhatIsNeitherFileNorDirectory)
{
    const ScratchDirectory dir("atlas_editor_odd");

    std::filesystem::create_symlink(
        dir.pathIn("nothing-here"), dir.pathIn("dangling"));

    EXPECT_EQ(
        entriesIn(dir.string()),
        (std::vector<FileEntry>{parent()}));
}

TEST(FileListTest, EntriesIn_NamesAFileTooLongToSitInsideAString)
{
    const ScratchDirectory dir("atlas_editor_long");
    const std::string named =
        "a-sprite-sheet-with-a-very-long-name-indeed.png";

    dir.write(named, "x");

    EXPECT_EQ(
        entriesIn(dir.string()),
        (std::vector<FileEntry>{parent(), FileEntry{.name = named}}));
}

TEST(FileListTest, EntriesIn_NamesADirectoryTooLongToSitInsideAString)
{
    const ScratchDirectory dir("atlas_editor_longdir");
    const std::string named =
        "a-directory-with-a-very-long-name-indeed-here";

    std::filesystem::create_directories(dir.pathIn(named));

    EXPECT_EQ(
        entriesIn(dir.string()),
        (std::vector<FileEntry>{
            parent(), FileEntry{.name = named, .directory = true}}));
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
