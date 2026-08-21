#include <gtest/gtest.h>

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

#include "antwika/app/SpawnDetached.hpp"

using antwika::app::spawnDetached;

namespace
{
    [[nodiscard]] bool appeared(const std::filesystem::path &path)
    {
        for (auto tries = 0; tries < 200; ++tries)
        {
            if (std::filesystem::exists(path))
            {
                return true;
            }

            std::this_thread::sleep_for(
                std::chrono::milliseconds{10});
        }

        return false;
    }

#ifdef _WIN32

    [[nodiscard]] std::string shellProgram()
    {
        const auto *const fromEnvironment = std::getenv("ComSpec");

        return fromEnvironment != nullptr
                   ? fromEnvironment
                   : "C:\\Windows\\System32\\cmd.exe";
    }

#endif

    [[nodiscard]] std::filesystem::path markPath(
        const std::string &name)
    {
        return std::filesystem::temp_directory_path()
               / ("antwika-spawn-" + name + ".mark");
    }
}

TEST(SpawnDetachedTest, SpawnDetached_StartsTheProgramItIsGiven)
{
    const auto mark = markPath("started");

    std::filesystem::remove(mark);

#ifdef _WIN32
    ASSERT_TRUE(spawnDetached(
        shellProgram(), {"/c", "copy", "/y", "NUL", mark.string()}));
#else
    ASSERT_TRUE(spawnDetached(
        "/bin/sh", {"-c", "printf hi > " + mark.string()}));
#endif
    EXPECT_TRUE(appeared(mark));

    std::filesystem::remove(mark);
}

TEST(SpawnDetachedTest, SpawnDetached_RefusesAProgramThatIsNotThere)
{
    EXPECT_FALSE(spawnDetached(
        (std::filesystem::temp_directory_path() / "antwika-no-such")
            .string(),
        {}));
}

TEST(SpawnDetachedTest, SpawnDetached_RefusesAPathThatIsNotAProgram)
{
    const auto plain = markPath("plain");

    {
        std::ofstream outputStream(plain);

        outputStream << "not a program";
    }

    EXPECT_FALSE(spawnDetached(plain.string(), {}));

    std::filesystem::remove(plain);
}
