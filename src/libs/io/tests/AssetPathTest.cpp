#include <gtest/gtest.h>

#include <filesystem>
#include <string>

#include <antwika/io/AssetPath.hpp>

using antwika::io::getAssetPath;
using antwika::io::getExecutableDirectory;

namespace
{

    TEST(AssetPathTest, ExecutableDirectory_IsAnAbsoluteDirectory)
    {
        const std::filesystem::path directory = getExecutableDirectory();

        EXPECT_TRUE(directory.is_absolute());
        EXPECT_TRUE(std::filesystem::is_directory(directory));
    }

    TEST(AssetPathTest, AssetPath_SitsBesideTheExecutable)
    {
        const std::filesystem::path expectedPath =
            std::filesystem::path(getExecutableDirectory()) / "atlas.png";

        EXPECT_EQ(getAssetPath("atlas.png"), expectedPath.string());
    }

    TEST(AssetPathTest, AssetPath_GivesTheDirectoryForAnEmptyName)
    {
        const std::filesystem::path expectedPath =
            std::filesystem::path(getExecutableDirectory()) / "";

        EXPECT_EQ(getAssetPath(""), expectedPath.string());
    }
}
