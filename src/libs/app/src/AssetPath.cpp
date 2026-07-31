#include "antwika/app/AssetPath.hpp"

#include <filesystem>
#include <string>
#include <string_view>

#ifdef _WIN32
#include <system_error>

#include <windows.h>
#endif

// The one place under src/ that names an operating system.
// Where a process's own executable is has no portable answer at all.
// antwika::app is the right home for it.
// This is the module that opens files, which no library below it does.
namespace antwika::app
{

#ifdef _WIN32

    std::string executableDirectory()
    {
        // GetModuleFileNameW fills whatever buffer it is handed.
        // It reports the size it was handed when the name did not fit.
        // An exact fit reports that too, so the two cannot be told apart.
        // Hence a buffer that grows until the answer is shorter.
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
        // Read rather than followed: the link names the executable.
        // An asset sits in the directory holding it.
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

} // namespace antwika::app
