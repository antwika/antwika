#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <system_error>

namespace antwika::life::tests
{

    /**
     * @brief A temporary file path that removes its file on scope exit.
     *
     * The record-then-replay tests here go through the real
     * saveReplayFile()/loadReplayFile() pair rather than a stringstream,
     * because the filtering those do is what main.cpp relies on. That
     * needs somewhere to put the file, and something to clean it up.
     */
    class ScratchFile
    {
    public:
        /**
         * @brief Name a file under the system's temporary directory.
         * @param name File name, unique among this suite's scratch files.
         */
        explicit ScratchFile(std::string_view name)
            : path(std::filesystem::temp_directory_path() / name)
        {
        }

        /**
         * @brief Remove the file, if it was ever written.
         *
         * The non-throwing overload, since a destructor that throws calls
         * std::terminate.
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
         * @brief Get the path, for the reader and writer under test.
         * @return The path as a string.
         */
        [[nodiscard]] std::string string() const
        {
            return path.string();
        }

    private:
        std::filesystem::path path;
    };

} // namespace antwika::life::tests
