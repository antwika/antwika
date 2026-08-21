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

    class ScratchFile final
    {
    public:
        explicit ScratchFile(std::string_view prefix)
            : where(scratchPath(prefix))
        {
        }

        ~ScratchFile()
        {
            std::error_code errorCode;
            std::filesystem::remove(where, errorCode);
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

}
