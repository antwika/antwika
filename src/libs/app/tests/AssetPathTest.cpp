#include <gtest/gtest.h>

#include <filesystem>
#include <string>

#include <antwika/app/AssetPath.hpp>

using antwika::app::assetPath;
using antwika::app::executableDirectory;

namespace
{

    TEST(AssetPathTest, ExecutableDirectory_IsAnAbsoluteDirectory)
    {
        const std::filesystem::path directory = executableDirectory();

        EXPECT_TRUE(directory.is_absolute());
        EXPECT_TRUE(std::filesystem::is_directory(directory));
    }

    TEST(AssetPathTest, AssetPath_SitsBesideTheExecutable)
    {
        const std::filesystem::path expected =
            std::filesystem::path(executableDirectory()) / "atlas.png";

        EXPECT_EQ(assetPath("atlas.png"), expected.string());
    }

    TEST(AssetPathTest, AssetPath_GivesTheDirectoryForAnEmptyName)
    {
        const std::filesystem::path expected =
            std::filesystem::path(executableDirectory()) / "";

        EXPECT_EQ(assetPath(""), expected.string());
    }
}
