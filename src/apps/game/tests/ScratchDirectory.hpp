#pragma once

#include <filesystem>
#include <string>
#include <string_view>

#include <gtest/gtest.h>

namespace antwika::game::tests
{

    /**
     * @brief Name a scratch directory for the test case running now.
     * @param prefix A name for the suite, to keep suites apart.
     * @return A path under the system's temporary directory.
     *
     * CTest registers every case as its own process, so a directory
     * named for the fixture alone is shared by every case of that
     * fixture running at once -- and a fixture that clears the
     * directory on construction then wipes a neighbour's files while
     * that neighbour is mid-run.
     * Naming it for the case as well makes the isolation real rather
     * than apparent, which a serial run cannot tell apart.
     */
    [[nodiscard]] inline std::filesystem::path scratchDirectory(
        std::string_view prefix)
    {
        const auto *const running
            = ::testing::UnitTest::GetInstance()->current_test_info();

        auto name = std::string{prefix};
        name += running->name();

        return std::filesystem::temp_directory_path() / name;
    }

} // namespace antwika::game::tests
