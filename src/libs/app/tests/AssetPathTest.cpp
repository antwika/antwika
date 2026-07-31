#include <filesystem>
#include <string>

#include <gtest/gtest.h>

#include <antwika/app/AssetPath.hpp>

using antwika::app::assetPath;
using antwika::app::executableDirectory;

namespace
{
    // The test binary is the running executable.
    // So what these assert about is themselves, and that is the point.
    // A test that stubbed the answer out would be testing the stub.

    TEST(AssetPathTest, ExecutableDirectoryIsAnAbsoluteDirectory)
    {
        const std::filesystem::path directory = executableDirectory();

        EXPECT_TRUE(directory.is_absolute());
        EXPECT_TRUE(std::filesystem::is_directory(directory));
    }

    TEST(AssetPathTest, AnAssetSitsBesideTheExecutable)
    {
        const std::filesystem::path expected =
            std::filesystem::path(executableDirectory()) / "atlas.png";

        EXPECT_EQ(assetPath("atlas.png"), expected.string());
    }

    // Naming nothing is not an error.
    // Answering the directory itself is all it could mean.
    TEST(AssetPathTest, AnEmptyNameIsTheDirectoryItself)
    {
        const std::filesystem::path expected =
            std::filesystem::path(executableDirectory()) / "";

        EXPECT_EQ(assetPath(""), expected.string());
    }
} // namespace
