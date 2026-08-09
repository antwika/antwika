#include "antwika/game/SaveGameFile.hpp"

#include <nlohmann/json.hpp>

#include <antwika/config/ConfigDocument.hpp>
#include <antwika/io/File.hpp>

#include "antwika/game/SaveFormatError.hpp"

namespace antwika::game
{

    namespace
    {
    }

    void writeSaveGame(const SaveGame &save, std::ostream &out)
    {
        antwika::config::writeConfig(saveGameToJson(save), out);
    }

    SaveGame readSaveGame(std::istream &in)
    {
        return saveGameFromJson(
            antwika::config::parseAs<SaveFormatError>(in));
    }

    void saveGameFile(const SaveGame &save, const std::string &path)
    {
        io::writeFileAs<SaveFormatError>(
            path, "a save", [&save](std::ostream &out) {
                writeSaveGame(save, out);
            });
    }

    SaveGame loadGameFile(const std::string &path)
    {
        auto file = io::openToReadAs<SaveFormatError>(path, "a save");

        return readSaveGame(file);
    }

    std::optional<SaveGame> loadGameFileIfNamed(
        const std::optional<std::string> &path)
    {
        if (!path.has_value())
        {
            return std::nullopt;
        }

        return loadGameFile(*path);
    }

    void saveGameFileIfNamed(
        const SaveGame &save, const std::optional<std::string> &path)
    {
        if (!path.has_value())
        {
            return;
        }

        saveGameFile(save, *path);
    }

}
