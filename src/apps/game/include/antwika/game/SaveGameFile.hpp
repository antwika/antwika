#pragma once

#include <istream>
#include <optional>
#include <ostream>
#include <string>

#include "antwika/game/SaveGame.hpp"

namespace antwika::game
{

    void writeSaveGame(const SaveGame &save, std::ostream &out);

    [[nodiscard]] SaveGame readSaveGame(std::istream &in);

    void saveGameFile(const SaveGame &save, const std::string &path);

    [[nodiscard]] SaveGame loadGameFile(const std::string &path);

    [[nodiscard]] std::optional<SaveGame> loadGameFileIfNamed(
        const std::optional<std::string> &path);

    void saveGameFileIfNamed(
        const SaveGame &save, const std::optional<std::string> &path);

}
