#include <gtest/gtest.h>

#include <filesystem>
#include <string>

#include <antwika/io/AssetPath.hpp>

using antwika::io::assetPath;
using antwika::io::executableDirectory;

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
        const std::filesystem::path expectedPath =
            std::filesystem::path(executableDirectory()) / "atlas.png";

        EXPECT_EQ(assetPath("atlas.png"), expectedPath.string());
    }

    TEST(AssetPathTest, AssetPath_GivesTheDirectoryForAnEmptyName)
    {
        const std::filesystem::path expectedPath =
            std::filesystem::path(executableDirectory()) / "";

        EXPECT_EQ(assetPath(""), expectedPath.string());
    }
}
