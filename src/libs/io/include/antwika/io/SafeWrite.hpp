#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <system_error>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

namespace antwika::io
{

    inline constexpr std::string_view kBackupSuffix = ".bak1";

    inline constexpr std::string_view kWritingSuffix = ".writing";

    [[nodiscard]] inline std::string writingPathFor(
        const std::string &path)
    {
        return path + std::string(kWritingSuffix);
    }

    [[nodiscard]] inline std::string backupPathFor(
        const std::string &path)
    {
        return path + std::string(kBackupSuffix);
    }

    inline void syncToDisk(const std::string &path) noexcept
    {
#ifdef _WIN32
        const auto file = _open(path.c_str(), _O_RDWR | _O_BINARY);

        if (file != -1)
        {
            _commit(file);
            _close(file);
        }
#else
        const auto file = ::open(path.c_str(), O_RDONLY);

        if (file != -1)
        {
            ::fsync(file);
            ::close(file);
        }
#endif
    }

    template <typename ErrorT>
    void putInPlaceKeepingBackup(
        const std::string &text,
        const std::string &path,
        std::string_view name)
    {
        syncToDisk(text);

        std::error_code errorCode;

        if (std::filesystem::exists(path, errorCode))
        {
            std::filesystem::rename(
                path, backupPathFor(path), errorCode);

            if (errorCode)
            {
                throw ErrorT(
                    std::string(name) + ": could not move "
                    + path + " aside to " + backupPathFor(path)
                    + ": " + errorCode.message());
            }
        }

        std::filesystem::rename(text, path, errorCode);

        if (errorCode)
        {
            std::error_code putBackCode;
            std::filesystem::rename(
                backupPathFor(path), path, putBackCode);

            throw ErrorT(
                std::string(name) + ": wrote " + text
                + " but could not move it into place: "
                + errorCode.message());
        }
    }

}
