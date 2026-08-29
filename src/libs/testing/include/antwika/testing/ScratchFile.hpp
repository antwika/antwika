#pragma once

#include <filesystem>
#include <string_view>
#include <system_error>

#include "antwika/testing/ScratchEntry.hpp"

namespace antwika::testing
{

    class ScratchFile final : public ScratchEntry
    {
    public:
        explicit ScratchFile(std::string_view prefix)
            : ScratchEntry(prefix)
        {
        }

        ~ScratchFile()
        {
            std::error_code errorCode;
            std::filesystem::remove(where, errorCode);
        }

        void write(std::string_view text) const
        {
            writeText(where, text);
        }
    };

}
