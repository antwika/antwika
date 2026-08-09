#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace antwika::game
{

    inline constexpr std::string_view kSaveExtension = ".save.json";

    [[nodiscard]] std::string saveGamePath(
        std::string_view directory, std::string_view name);

    [[nodiscard]] std::vector<std::string> listSaveGames(
        std::string_view directory);

}
