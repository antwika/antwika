#pragma once

#include <filesystem>
#include <string>
#include <string_view>

#include <unistd.h>

#include <gtest/gtest.h>

namespace antwika::game::tests
{

    /**
     * @brief Name a scratch directory nothing else will ever name.
     * @param prefix A name for the suite, to keep suites apart.
     * @return A path under the system's temporary directory.
     *
     * Two things have to be true, and the case name alone gives only
     * the first.
     *
     * CTest registers every case as its own process, so a directory
     * named for the fixture alone is shared by every case of that
     * fixture running at once -- and a fixture that clears the
     * directory on construction then wipes a neighbour's files while
     * that neighbour is mid-run.
     *
     * The process id is what stops a *re-run* colliding with itself.
     * A name built from the case alone is the same path every run, so
     * a fixture that removes it and immediately recreates it is asking
     * the filesystem to retire and reissue one directory entry with no
     * pause in between -- which an overlay filesystem under load does
     * not always do in that order, leaving a directory that exists to
     * `create_directories` and is gone by the time a file is opened in
     * it.
     * A path that has never existed before cannot be caught halfway
     * through being removed.
     */
    [[nodiscard]] inline std::filesystem::path scratchDirectory(
        std::string_view prefix)
    {
        const auto *const running
            = ::testing::UnitTest::GetInstance()->current_test_info();

        auto name = std::string{prefix};
        name += running->name();
        name += '.';
        name += std::to_string(::getpid());

        return std::filesystem::temp_directory_path() / name;
    }

} // namespace antwika::game::tests
