#include "antwika/game/SaveDirectory.hpp"

#include <algorithm>
#include <filesystem>
#include <system_error>

namespace antwika::game
{

    std::string saveGamePath(
        std::string_view directory, std::string_view name)
    {
        return std::string(directory) + "/" + std::string(name)
               + std::string(kSaveExtension);
    }

    std::vector<std::string> listSaveGames(std::string_view directory)
    {
        std::vector<std::string> names;

        // The non-throwing overload.
        // A directory that is not there is a session with no saves.
        std::error_code failed;
        const std::filesystem::directory_iterator entries(
            std::filesystem::path(directory), failed);

        if (failed)
        {
            return names;
        }

        for (const auto &entry : entries)
        {
            const std::string file = entry.path().filename().string();

            if (file.size() <= kSaveExtension.size()
                || !file.ends_with(kSaveExtension))
            {
                continue;
            }

            names.push_back(
                file.substr(0, file.size() - kSaveExtension.size()));
        }

        // Which option a click lands on is a function of the order.
        // A directory iterator promises none, so one is imposed here.
        std::sort(names.begin(), names.end());
        return names;
    }

} // namespace antwika::game
