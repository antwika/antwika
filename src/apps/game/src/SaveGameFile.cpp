#include "antwika/game/SaveGameFile.hpp"

#include <fstream>

#include <nlohmann/json.hpp>

#include "antwika/game/SaveFormatError.hpp"

namespace antwika::game
{

    namespace
    {
        // Two spaces, one member a line.
        // Enough to diff a save against the next version of itself.
        // That is the whole reason this format is not compact.
        constexpr int kIndent = 2;
    } // namespace

    void writeSaveGame(const SaveGame &save, std::ostream &out)
    {
        out << saveGameToJson(save).dump(kIndent) << '\n';
    }

    SaveGame readSaveGame(std::istream &in)
    {
        nlohmann::json document;
        try
        {
            in >> document;
        }
        catch (const nlohmann::json::exception &error) // GCOVR_EXCL_LINE
        {
            throw SaveFormatError(
                std::string("antwika::game: save is not valid JSON: ")
                + error.what());
        }

        return saveGameFromJson(document);
    }

    void saveGameFile(const SaveGame &save, const std::string &path)
    {
        std::ofstream file(path);
        if (!file.is_open())
        {
            throw SaveFormatError(
                "antwika::game: could not open a save to write: " + path);
        }

        writeSaveGame(save, file);

        // Flushed here rather than by the destructor, which cannot say.
        // A full disk fails on the flush, not on the open.
        file.flush();
        if (!file)
        {
            throw SaveFormatError(
                "antwika::game: could not write a save: " + path);
        }
    }

    SaveGame loadGameFile(const std::string &path)
    {
        std::ifstream file(path);

        // A file that is not there is not a malformed document.
        // Unchecked it reaches the parser as an empty stream.
        // Which reports a missing save as invalid JSON.
        if (!file.is_open())
        {
            throw SaveFormatError(
                "antwika::game: could not open a save to read: " + path);
        }

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
