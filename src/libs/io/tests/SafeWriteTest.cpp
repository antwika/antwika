#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

#include <antwika/testing/ScratchDirectory.hpp>

#include "antwika/io/SafeWrite.hpp"
#include "antwika/io/ScratchFile.hpp"

using antwika::io::backupPathFor;
using antwika::io::putInPlaceKeepingBackup;
using antwika::testing::ScratchDirectory;

namespace
{

    void writeText(const std::string &path, const std::string &text)
    {
        std::ofstream file(path);
        file << text;
    }

    [[nodiscard]] std::string textOf(const std::string &path)
    {
        std::ifstream file(path);
        std::string text;
        std::getline(file, text);

        return text;
    }

}

TEST(SafeWriteTest, PutInPlaceKeepingBackup_MovesTheWrittenTextOver)
{
    const ScratchDirectory scratch("safe-write");
    const auto path = scratch.pathIn("map.json");
    const auto writtenPath = scratch.pathIn("map.json.writingFile");

    writeText(path, "before");
    writeText(writtenPath, "after");

    putInPlaceKeepingBackup<std::runtime_error>(
        writtenPath, path, "test");

    EXPECT_EQ(textOf(path), "after");
    EXPECT_EQ(textOf(backupPathFor(path)), "before");
}

TEST(SafeWriteTest, PutInPlaceKeepingBackup_LeavesTheFileWhereItWas)
{
    const ScratchDirectory scratch("safe-write");
    const auto path = scratch.pathIn("map.json");
    const auto missingPath = scratch.pathIn("no-such.writingFile");

    writeText(path, "before");

    EXPECT_THROW(
        putInPlaceKeepingBackup<std::runtime_error>(
            missingPath, path, "test"),
        std::runtime_error);

    ASSERT_TRUE(std::filesystem::exists(path));
    EXPECT_EQ(textOf(path), "before");
}

TEST(ScratchFileTest, ScratchFile_RemovesItsPathWhenTheScopeEnds)
{
    const ScratchDirectory scratch("scratch-file");
    const auto path = scratch.pathIn("map.json.writingFile");

    {
        const antwika::io::ScratchFile writingFile{path};

        writeText(path, "part way");

        ASSERT_TRUE(std::filesystem::exists(path));
    }

    EXPECT_FALSE(std::filesystem::exists(path));
}

TEST(ScratchFileTest, Keep_LeavesThePathAlone)
{
    const ScratchDirectory scratch("scratch-file");
    const auto path = scratch.pathIn("map.json.writingFile");

    {
        antwika::io::ScratchFile writingFile{path};

        writeText(path, "finished");
        writingFile.keep();
    }

    EXPECT_TRUE(std::filesystem::exists(path));
}
