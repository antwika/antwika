#include "antwika/app/AssetPath.hpp"

#include <filesystem>
#include <string>
#include <string_view>

#ifdef _WIN32
#include <system_error>

#include <windows.h>
#endif

namespace antwika::app
{

#ifdef _WIN32

    std::string executableDirectory()
    {
        std::wstring buffer(MAX_PATH, L'\0');

        for (;;)
        {
            const DWORD written = ::GetModuleFileNameW(
                nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));

            if (written == 0)
            {
                throw std::filesystem::filesystem_error(
                    "could not find the running executable",
                    std::error_code(static_cast<int>(::GetLastError()),
                        std::system_category()));
            }

            if (written < buffer.size())
            {
                buffer.resize(written);
                break;
            }

            buffer.resize(buffer.size() * 2);
        }

        return std::filesystem::path(buffer).parent_path().string();
    }

#else

    std::string executableDirectory()
    {
        return std::filesystem::read_symlink("/proc/self/exe")
            .parent_path()
            .string();
    }

#endif

    std::string assetPath(std::string_view name)
    {
        return (std::filesystem::path(executableDirectory()) /
            std::filesystem::path(name))
            .string();
    }

}
