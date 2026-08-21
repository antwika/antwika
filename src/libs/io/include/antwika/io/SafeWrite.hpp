#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <system_error>

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

    template <typename ErrorT>
    void putInPlaceKeepingBackup(
        const std::string &text,
        const std::string &path,
        std::string_view name)
    {
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
            throw ErrorT(
                std::string(name) + ": wrote " + text
                + " but could not move it into place: "
                + errorCode.message());
        }
    }

}
