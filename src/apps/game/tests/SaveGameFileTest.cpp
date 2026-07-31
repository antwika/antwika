#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>

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
using antwika::game::WalkerView;
using antwika::game::writeSaveGame;

namespace
{
    // Removes its backing file on scope exit.
    // That way a failing assertion leaves no stray temp files behind.
    class ScratchFile
    {
    public:
        explicit ScratchFile(std::string_view name)
            : path(std::filesystem::temp_directory_path() / name)
        {
        }

        ~ScratchFile()
        {
            // The error_code overload, not the throwing one.
            // A destructor is implicitly noexcept.
            std::error_code ignored;
            std::filesystem::remove(path, ignored);
        }

        ScratchFile(const ScratchFile &) = delete;
        ScratchFile(ScratchFile &&) = delete;
        ScratchFile &operator=(const ScratchFile &) = delete;
        ScratchFile &operator=(ScratchFile &&) = delete;

        [[nodiscard]] std::string string() const
        {
            return path.string();
        }

    private:
        std::filesystem::path path;
    };

    SaveGame populated()
    {
        SaveGame save;
        save.state = GameState{.ticksProcessed = 5, .score = 3};
        save.extent = GridExtent{.width = 16, .height = 16};
        save.camera = Camera(Point{.x = 4, .y = 8}, 2);
        save.paths = {Cell{.x = 2, .y = 2}};
        save.walkers = {
            WalkerView{.at = {.x = 2, .y = 2}, .facing = Direction::North}};
        save.seed = 12345;
        return save;
    }
} // namespace

TEST(SaveGameFileTest, RoundTripsThroughAStream)
{
    const auto original = populated();
    std::stringstream stream;

    writeSaveGame(original, stream);

    EXPECT_EQ(readSaveGame(stream), original);
}

TEST(SaveGameFileTest, WritesAnIndentedDocument)
{
    std::stringstream stream;

    writeSaveGame(populated(), stream);

    EXPECT_NE(stream.str().find("\n  \"magic\""), std::string::npos);
}

TEST(SaveGameFileTest, RoundTripsThroughAFile)
{
    const ScratchFile file("antwika_game_save_roundtrip.json");
    const auto original = populated();

    saveGameFile(original, file.string());

    EXPECT_EQ(loadGameFile(file.string()), original);
}

TEST(SaveGameFileTest, RejectsAStreamThatIsNotJson)
{
    std::stringstream stream("this is not a save");

    EXPECT_THROW((void)readSaveGame(stream), SaveFormatError);
}

TEST(SaveGameFileTest, RejectsAFileThatIsNotThere)
{
    const ScratchFile file("antwika_game_save_absent.json");

    EXPECT_THROW((void)loadGameFile(file.string()), SaveFormatError);
}

TEST(SaveGameFileTest, RejectsAPathThatCannotBeWritten)
{
    EXPECT_THROW(
        saveGameFile(populated(), "/nonexistent-directory/save.json"),
        SaveFormatError);
}

TEST(SaveGameFileTest, RejectsAMalformedFile)
{
    const ScratchFile file("antwika_game_save_malformed.json");
    {
        std::ofstream out(file.string());
        out << "{ \"magic\": ";
    }

    EXPECT_THROW((void)loadGameFile(file.string()), SaveFormatError);
}

// Opening is not writing: a full disk fails only once bytes are flushed.
// /dev/full is the portable-enough way to make that happen on purpose.
TEST(SaveGameFileTest, ReportsAWriteThatFailsAfterTheOpen)
{
    if (!std::filesystem::exists("/dev/full"))
    {
        GTEST_SKIP() << "no /dev/full to fill";
    }

    EXPECT_THROW(saveGameFile(populated(), "/dev/full"), SaveFormatError);
}
