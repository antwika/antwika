#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

#include <antwika/testing/ScratchDirectory.hpp>

#include "antwika/editor/editor/FileChooser.hpp"
#include "antwika/editor/fakes/FakeNotices.hpp"
#include "antwika/editor/ui/MapPicker.hpp"

using antwika::editor::FileChooser;
using antwika::editor::fakes::FakeNotices;
using antwika::testing::ScratchDirectory;

namespace
{

    void putFileAt(const std::filesystem::path &path)
    {
        std::ofstream stream(path);

        stream << "{}";
    }

    class FileChooserTest : public ::testing::Test
    {
    protected:
        FileChooserTest()
        {
            mapsFolder = std::filesystem::path(scratch.pathIn("maps"));
            std::filesystem::create_directories(mapsFolder / "sub");
            putFileAt(mapsFolder / "a.json");
            putFileAt(mapsFolder / "b.json");
            putFileAt(mapsFolder / "note.txt");
        }

        ScratchDirectory scratch{"file-chooser"};
        std::filesystem::path mapsFolder;
        FileChooser chooser;
        FakeNotices notices;
    };

}

TEST_F(FileChooserTest, Open_StartsBesideTheMapWithItsName)
{
    chooser.open((mapsFolder / "a.json").string(), "", false);

    ASSERT_TRUE(chooser.fileDialog.has_value());
    EXPECT_FALSE(chooser.fileDialog->isSaveMode);
    EXPECT_EQ(
        chooser.fileDialog->folder,
        std::filesystem::absolute(mapsFolder).string());
    EXPECT_EQ(chooser.fileDialog->fileName, "a.json");
    EXPECT_EQ(chooser.folderEntries, (std::vector<std::string>{"sub"}));
    EXPECT_EQ(
        chooser.mapEntries, (std::vector<std::string>{"a.json", "b.json"}));
}

TEST_F(FileChooserTest, Open_FallsBackToTheStartPathWithNoNameToKeep)
{
    chooser.open("", (mapsFolder / "start.json").string(), true);

    ASSERT_TRUE(chooser.fileDialog.has_value());
    EXPECT_TRUE(chooser.fileDialog->isSaveMode);
    EXPECT_TRUE(chooser.fileDialog->fileName.empty());
    EXPECT_EQ(
        chooser.fileDialog->folder,
        std::filesystem::absolute(mapsFolder).string());
}

TEST_F(FileChooserTest, ListFolder_CapsEachListAtThePickerRoomForIt)
{
    for (std::size_t index = 0;
         index < antwika::editor::kMaxPicked + 6;
         ++index)
    {
        const auto tail = std::to_string(100 + index);

        std::filesystem::create_directories(mapsFolder / ("deep-" + tail));
        putFileAt(mapsFolder / ("map-" + tail + ".json"));
    }

    chooser.open((mapsFolder / "a.json").string(), "", false);

    EXPECT_EQ(chooser.folderEntries.size(), antwika::editor::kMaxPicked);
    EXPECT_EQ(chooser.mapEntries.size(), antwika::editor::kMaxPicked);
    EXPECT_LE(
        chooser.folderEntries.size() + chooser.mapEntries.size(),
        antwika::editor::kMaxPickedRows);
}

TEST_F(FileChooserTest, ListFolder_LeavesTheListsEmptyOnAMissingFolder)
{
    chooser.open((mapsFolder / "a.json").string(), "", false);
    chooser.listFolder((mapsFolder / "no-such-folder").string());

    EXPECT_TRUE(chooser.folderEntries.empty());
    EXPECT_TRUE(chooser.mapEntries.empty());
}

TEST_F(FileChooserTest, Cancel_DropsTheDialog)
{
    chooser.open((mapsFolder / "a.json").string(), "", true);
    chooser.cancel();

    EXPECT_FALSE(chooser.fileDialog.has_value());
}

TEST_F(FileChooserTest, Confirm_GivesNothingWhenNoDialogIsUp)
{
    EXPECT_FALSE(chooser.confirm(notices).has_value());
    EXPECT_TRUE(notices.statusTexts.empty());
}

TEST_F(FileChooserTest, Confirm_AsksForANameBeforeSaving)
{
    chooser.open("", (mapsFolder / "start.json").string(), true);

    EXPECT_FALSE(chooser.confirm(notices).has_value());
    ASSERT_EQ(notices.statusTexts.size(), 1U);
    EXPECT_EQ(notices.statusTexts.front(), "the map needs a name");
    EXPECT_TRUE(chooser.fileDialog.has_value());
}

TEST_F(FileChooserTest, Confirm_HandsBackTheChosenPathAndCloses)
{
    chooser.open((mapsFolder / "a.json").string(), "", true);

    const auto choice = chooser.confirm(notices);

    ASSERT_TRUE(choice.has_value());
    EXPECT_TRUE(choice->isSaveMode);
    EXPECT_EQ(
        choice->path,
        (std::filesystem::absolute(mapsFolder) / "a.json").string());
    EXPECT_FALSE(chooser.fileDialog.has_value());
}

TEST_F(FileChooserTest, Confirm_GivesABareNameTheMapExtension)
{
    chooser.open((mapsFolder / "a.json").string(), "", true);
    chooser.fileDialog->fileName = "fresh";

    const auto choice = chooser.confirm(notices);

    ASSERT_TRUE(choice.has_value());
    EXPECT_EQ(
        choice->path,
        (std::filesystem::absolute(mapsFolder) / "fresh.json").string());
}
