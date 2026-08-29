#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <system_error>

#include "antwika/testing/ScratchEntry.hpp"

namespace antwika::testing
{

    class ScratchDirectory final : public ScratchEntry
    {
    public:
        explicit ScratchDirectory(std::string_view prefix)
            : ScratchEntry(prefix)
        {
            std::filesystem::create_directories(where);
        }

        ~ScratchDirectory()
        {
            std::error_code errorCode;
            std::filesystem::remove_all(where, errorCode);
        }

        [[nodiscard]] std::string pathIn(std::string_view name) const
        {
            return (where / name).string();
        }

        void write(std::string_view name, std::string_view text) const
        {
            writeText(where / name, text);
        }
    };

}
