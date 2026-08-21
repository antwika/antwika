#pragma once

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <system_error>
#include <unistd.h>

#include "antwika/testing/ScratchPath.hpp"

namespace antwika::testing
{

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
            std::error_code errorCode;
            std::filesystem::remove_all(where, errorCode);
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
