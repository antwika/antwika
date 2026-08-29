#pragma once

#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <system_error>

#include "antwika/testing/ScratchPath.hpp"

namespace antwika::testing
{

    class ScratchEntry
    {
    public:
        ScratchEntry(const ScratchEntry &) = delete;
        ScratchEntry(ScratchEntry &&) = delete;

        ScratchEntry &operator=(const ScratchEntry &) = delete;
        ScratchEntry &operator=(ScratchEntry &&) = delete;

        [[nodiscard]] const std::filesystem::path &getPath() const noexcept
        {
            return where;
        }

        [[nodiscard]] std::string getString() const
        {
            return where.string();
        }

    protected:
        explicit ScratchEntry(std::string_view prefix)
            : where(getScratchPath(prefix))
        {
        }

        ~ScratchEntry() = default;

        static void writeText(
            const std::filesystem::path &file, std::string_view text)
        {
            std::ofstream stream(file);
            stream << text;
            stream.close();

            if (!stream)
            {
                throw std::filesystem::filesystem_error(
                    "could not write the scratch text",
                    file,
                    std::make_error_code(std::errc::io_error));
            }
        }

        std::filesystem::path where;
    };

}
