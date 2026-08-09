#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <vector>

#include <antwika/testing/ScratchPath.hpp>

#include "antwika/game/SaveDirectory.hpp"

namespace
{

        using antwika::game::listSaveGames;
    using antwika::game::saveGamePath;

    class SaveDirectoryTest : public ::testing::Test
    {
    protected:
        SaveDirectoryTest()
            : directory(
                  antwika::testing::scratchPath("antwika_saves_"))
        {
            std::error_code ignored;
            std::filesystem::remove_all(directory, ignored);
        }

        ~SaveDirectoryTest() override
        {
            std::error_code ignored;
            std::filesystem::remove_all(directory, ignored);
        }

        SaveDirectoryTest(const SaveDirectoryTest &) = delete;
        SaveDirectoryTest(SaveDirectoryTest &&) = delete;

        SaveDirectoryTest &operator=(const SaveDirectoryTest &) = delete;
        SaveDirectoryTest &operator=(SaveDirectoryTest &&) = delete;

        void write(const std::string &file)
        {
            std::error_code ignored;
            std::filesystem::create_directories(directory, ignored);

            const std::ofstream out(directory / file);
        }

        [[nodiscard]] std::string path() const
        {
            return directory.string();
        }

        std::filesystem::path directory;
    };

    TEST_F(SaveDirectoryTest, SaveGamePath_JoinsTheNameAndTheExtension)
    {
        EXPECT_EQ(
            saveGamePath("saves", "town"), "saves/town.save.json");
    }

    TEST_F(SaveDirectoryTest, ListSaveGames_FindsNothingInADirectoryThatIsNot)
    {
        EXPECT_TRUE(listSaveGames(path()).empty());
    }

    TEST_F(SaveDirectoryTest, ListSaveGames_NamesEverySaveInOrder)
    {
        write("zeta.save.json");
        write("alpha.save.json");

        EXPECT_EQ(
            listSaveGames(path()),
            (std::vector<std::string>{"alpha", "zeta"}));
    }

    TEST_F(SaveDirectoryTest, ListSaveGames_SkipsAnythingElse)
    {
        write("demo.json");
        write("notes.txt");
        write("town.save.json");

        write(".save.json");

        write("a-long-enough-replay.json");

        EXPECT_EQ(
            listSaveGames(path()), (std::vector<std::string>{"town"}));
    }

}
