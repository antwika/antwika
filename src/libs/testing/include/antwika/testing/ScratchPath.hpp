#pragma once

#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <system_error>

#include <unistd.h>

#include <gtest/gtest.h>

namespace antwika::testing
{

    /**
     * @brief Name a scratch path nothing else will ever name.
     * @param prefix A name for the suite, to keep suites apart.
     * @return A path under the system's temporary directory.
     *
     * Two things have to be true, and the case name alone gives only
     * the first.
     *
     * CTest registers every case as its own process, so a path named
     * for the fixture alone is shared by every case of that fixture
     * running at once -- and a fixture that clears it on construction
     * then wipes a neighbour's files while that neighbour is mid-run.
     * That is not hypothetical: it is what made
     * `SessionPersistenceTest` flake at 22 failures in 160 paired runs
     * before every scratch path in the tree carried a pid.
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
    [[nodiscard]] inline std::filesystem::path scratchPath(
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

    /**
     * @brief A file under the temporary directory, removed on the way
     * out.
     *
     * Removing on scope exit is what keeps a *failing* assertion from
     * leaving debris behind, which a test that tidied up at the end of
     * its body would not.
     *
     * The name carries the case and the pid, for scratchPath()'s
     * reason; a caller that wants the naming without the file uses
     * that function directly.
     */
    class ScratchFile final
    {
    public:
        /**
         * @brief Name a file for this case.
         * @param prefix A name for the suite, to keep suites apart.
         */
        explicit ScratchFile(std::string_view prefix)
            : where(scratchPath(prefix))
        {
        }

        ~ScratchFile()
        {
            // The error_code overload, not the throwing one.
            // A destructor is implicitly noexcept.
            std::error_code ignored;
            std::filesystem::remove(where, ignored);
        }

        ScratchFile(const ScratchFile &) = delete;
        ScratchFile(ScratchFile &&) = delete;

        ScratchFile &operator=(const ScratchFile &) = delete;
        ScratchFile &operator=(ScratchFile &&) = delete;

        /** @brief Where the file is, as a path. */
        [[nodiscard]] const std::filesystem::path &path() const noexcept
        {
            return where;
        }

        /** @brief Where the file is, as a string. */
        [[nodiscard]] std::string string() const
        {
            return where.string();
        }

        /**
         * @brief Put text in it.
         * @param text What to write.
         */
        void write(std::string_view text) const
        {
            std::ofstream file(where);
            file << text;
        }

    private:
        std::filesystem::path where;
    };

    /**
     * @brief A directory under the temporary directory, removed whole
     * on the way out.
     *
     * ScratchFile's sibling, on the same naming terms, for a case that
     * needs several files or a directory listing.
     */
    class ScratchDirectory final
    {
    public:
        /**
         * @brief Make a directory for this case.
         * @param prefix A name for the suite, to keep suites apart.
         */
        explicit ScratchDirectory(std::string_view prefix)
            : where(scratchPath(prefix))
        {
            std::filesystem::create_directories(where);
        }

        ~ScratchDirectory()
        {
            std::error_code ignored;
            std::filesystem::remove_all(where, ignored);
        }

        ScratchDirectory(const ScratchDirectory &) = delete;
        ScratchDirectory(ScratchDirectory &&) = delete;

        ScratchDirectory &operator=(const ScratchDirectory &) = delete;
        ScratchDirectory &operator=(ScratchDirectory &&) = delete;

        /** @brief Where the directory is, as a path. */
        [[nodiscard]] const std::filesystem::path &path() const noexcept
        {
            return where;
        }

        /** @brief Where the directory is, as a string. */
        [[nodiscard]] std::string string() const
        {
            return where.string();
        }

        /**
         * @brief Name a file inside it.
         * @param name The file's name.
         * @return The path, as a string.
         */
        [[nodiscard]] std::string pathIn(std::string_view name) const
        {
            return (where / name).string();
        }

        /**
         * @brief Put text in a file inside it.
         * @param name The file's name.
         * @param text What to write.
         */
        void write(std::string_view name, std::string_view text) const
        {
            std::ofstream file(where / name);
            file << text;
        }

    private:
        std::filesystem::path where;
    };

} // namespace antwika::testing
