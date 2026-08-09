#include <gtest/gtest.h>

#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <unistd.h>

#include <antwika/testing/ScratchPath.hpp>

#include "antwika/game/SaveFormatError.hpp"
#include "antwika/game/SaveGameFile.hpp"

using antwika::game::Camera;
using antwika::game::Cell;
using antwika::game::Direction;
using antwika::game::GameState;
using antwika::game::GridExtent;
using antwika::game::loadGameFile;
using antwika::game::Point;
using antwika::game::readSaveGame;
using antwika::game::SaveFormatError;
using antwika::game::SaveGame;
using antwika::game::saveGameFile;
using antwika::game::SavedWalker;
using antwika::game::writeSaveGame;

namespace
{
    SaveGame populated()
    {
        SaveGame save;
        save.state = GameState{.ticksProcessed = 5, .score = 3};
        save.extent = GridExtent{.width = 16, .height = 16};
        save.camera = Camera(Point{.x = 4, .y = 8}, 2);
        save.paths = {Cell{.x = 2, .y = 2}};
        save.walkers = {
            SavedWalker{.at = {.x = 2, .y = 2}, .facing = Direction::North}};
        save.seed = 12345;
        return save;
    }
}

TEST(SaveGameFileTest, ReadSaveGame_RoundTripsThroughAStream)
{
    const auto original = populated();
    std::stringstream stream;

    writeSaveGame(original, stream);

    EXPECT_EQ(readSaveGame(stream), original);
}

TEST(SaveGameFileTest, WriteSaveGame_WritesAnIndentedDocument)
{
    std::stringstream stream;

    writeSaveGame(populated(), stream);

    EXPECT_NE(stream.str().find("\n  \"magic\""), std::string::npos);
}

TEST(SaveGameFileTest, LoadGameFile_RoundTripsThroughAFile)
{
    const antwika::testing::ScratchFile file(
        "antwika_game_save_roundtrip.json");
    const auto original = populated();

    saveGameFile(original, file.string());

    EXPECT_EQ(loadGameFile(file.string()), original);
}

TEST(SaveGameFileTest, ReadSaveGame_RejectsAStreamThatIsNotJson)
{
    std::stringstream stream("this is not a save");

    EXPECT_THROW((void)readSaveGame(stream), SaveFormatError);
}

TEST(SaveGameFileTest, LoadGameFile_RejectsAFileThatIsNotThere)
{
    const antwika::testing::ScratchFile file("antwika_game_save_absent.json");

    EXPECT_THROW((void)loadGameFile(file.string()), SaveFormatError);
}

TEST(SaveGameFileTest, SaveGameFile_RejectsAPathThatCannotBeWritten)
{
    EXPECT_THROW(
        saveGameFile(populated(), "/nonexistent-directory/save.json"),
        SaveFormatError);
}

TEST(SaveGameFileTest, LoadGameFileIfNamed_RejectsAMalformedFile)
{
    const antwika::testing::ScratchFile file(
        "antwika_game_save_malformed.json");
    {
        std::ofstream out(file.string());
        out << "{ \"magic\": ";
    }

    EXPECT_THROW((void)loadGameFile(file.string()), SaveFormatError);
}

TEST(SaveGameFileTest, SaveGameFileIfNamed_DoesNothingUnnamed)
{
    EXPECT_FALSE(
        antwika::game::loadGameFileIfNamed(std::nullopt).has_value());

    EXPECT_NO_THROW(
        antwika::game::saveGameFileIfNamed(populated(), std::nullopt));
}

TEST(SaveGameFileTest, SaveGameFileIfNamed_RoundTripsANamedPath)
{
    const antwika::testing::ScratchFile file("antwika_game_save_if_named.json");
    const std::optional<std::string> path{file.string()};

    antwika::game::saveGameFileIfNamed(populated(), path);

    const auto loaded = antwika::game::loadGameFileIfNamed(path);

    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(*loaded, populated());
}
