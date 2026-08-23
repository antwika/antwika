#pragma once

#include <filesystem>
#include <string>
#include <system_error>
#include <utility>

namespace antwika::io
{

    class ScratchFile final
    {
    public:
        explicit ScratchFile(std::string path) noexcept
            : where(std::move(path))
        {
        }

        ScratchFile(const ScratchFile &) = delete;
        ScratchFile(ScratchFile &&) = delete;

        ScratchFile &operator=(const ScratchFile &) = delete;
        ScratchFile &operator=(ScratchFile &&) = delete;

        ~ScratchFile()
        {
            if (where.empty())
            {
                return;
            }

            std::error_code errorCode;
            std::filesystem::remove(where, errorCode);
        }

        [[nodiscard]] const std::string &getPath() const noexcept
        {
            return where;
        }

        void keep() noexcept
        {
            where.clear();
        }

    private:
        std::string where;
    };

}
