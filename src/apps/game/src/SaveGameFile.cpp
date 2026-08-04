#include "antwika/game/SaveGameFile.hpp"

#include <antwika/config/ConfigDocument.hpp>
#include <antwika/io/File.hpp>

#include <nlohmann/json.hpp>

#include "antwika/game/SaveFormatError.hpp"

namespace antwika::game
{

    namespace
    {
    } // namespace

    void writeSaveGame(const SaveGame &save, std::ostream &out)
    {
        antwika::config::writeConfig(saveGameToJson(save), out);
    }

    SaveGame readSaveGame(std::istream &in)
    {
        // The parse is antwika::config's.
        // So a malformed document reads the same way everywhere.
        return saveGameFromJson(
            antwika::config::parseAs<SaveFormatError>(in));
    }

    void saveGameFile(const SaveGame &save, const std::string &path)
    {
        // The open, the flush and the write refusal are antwika::io's.
        // That discipline is stated once, over there.
        io::writeFileAs<SaveFormatError>(
            path, "a save", [&save](std::ostream &out) {
                writeSaveGame(save, out);
            });
    }

    SaveGame loadGameFile(const std::string &path)
    {
        // A file that is not there is not a malformed document.
        // Unchecked it reaches the parser as an empty stream.
        // Which reports a missing save as invalid JSON.
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

} // namespace antwika::game
