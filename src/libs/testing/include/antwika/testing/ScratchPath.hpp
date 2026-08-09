#pragma once

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <system_error>
#include <unistd.h>

namespace antwika::testing
{

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

    class ScratchFile final
    {
    public:
        explicit ScratchFile(std::string_view prefix)
            : where(scratchPath(prefix))
        {
        }

        ~ScratchFile()
        {
            std::error_code ignored;
            std::filesystem::remove(where, ignored);
        }

        ScratchFile(const ScratchFile &) = delete;
        ScratchFile(ScratchFile &&) = delete;

        ScratchFile &operator=(const ScratchFile &) = delete;
        ScratchFile &operator=(ScratchFile &&) = delete;

        [[nodiscard]] const std::filesystem::path &path() const noexcept
        {
            return where;
        }

        [[nodiscard]] std::string string() const
        {
            return where.string();
        }

        void write(std::string_view text) const
        {
            std::ofstream file(where);
            file << text;
        }

    private:
        std::filesystem::path where;
    };

    class ScratchDirectory final
    {
    public:
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

        [[nodiscard]] const std::filesystem::path &path() const noexcept
        {
            return where;
        }

        [[nodiscard]] std::string string() const
        {
            return where.string();
        }

        [[nodiscard]] std::string pathIn(std::string_view name) const
        {
            return (where / name).string();
        }

        void write(std::string_view name, std::string_view text) const
        {
            std::ofstream file(where / name);
            file << text;
        }

    private:
        std::filesystem::path where;
    };

}
