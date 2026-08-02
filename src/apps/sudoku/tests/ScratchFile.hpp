#pragma once

#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <system_error>

namespace antwika::sudoku::tests
{

    /**
     * @brief A temporary file path that removes its file on scope exit.
     *
     * startingPuzzle() opens a path, so proving what it does with one
     * needs a real file somewhere and something to clean it up.
     * Line for line life::tests::ScratchFile, with a write() added,
     * since what this suite needs on disk is a puzzle rather than a
     * recording something else wrote.
     */
    class ScratchFile final
    {
    public:
        /**
         * @brief Name a file under the system's temporary directory.
         * @param name File name, unique among this suite's files.
         */
        explicit ScratchFile(std::string_view name)
            : path(std::filesystem::temp_directory_path() / name)
        {
        }

        /**
         * @brief Remove the file, if it was ever written.
         *
         * The non-throwing overload, since a destructor that throws
         * calls std::terminate.
         */
        ~ScratchFile()
        {
            std::error_code ignored;
            std::filesystem::remove(path, ignored);
        }

        ScratchFile(const ScratchFile &) = delete;
        ScratchFile(ScratchFile &&) = delete;

        ScratchFile &operator=(const ScratchFile &) = delete;
        ScratchFile &operator=(ScratchFile &&) = delete;

        /**
         * @brief Put some text in the file.
         * @param text What to write.
         */
        void write(std::string_view text) const
        {
            std::ofstream out{path};
            out << text;
        }

        /**
         * @brief Get the path, for the loader under test.
         * @return The path as a string.
         */
        [[nodiscard]] std::string string() const
        {
            return path.string();
        }

    private:
        std::filesystem::path path;
    };

} // namespace antwika::sudoku::tests
