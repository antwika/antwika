#include "antwika/tilemap/MapFile.hpp"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <string>

#include "antwika/tilemap/MapJson.hpp"
#include "antwika/tilemap/TileMapError.hpp"

namespace antwika::tilemap
{

    namespace
    {
        constexpr int kIndent = 2;

        [[nodiscard]] TileMapError fileError(
            const std::string &what,
            const std::filesystem::path &path)
        {
            return TileMapError(what + ": " + path.string());
        }
    }

    void saveMapFile(
        const std::filesystem::path &path, const TileMap &map)
    {
        std::ofstream out(path);

        if (!out)
        {
            throw fileError(
                "the map file cannot be opened for writing", path);
        }

        out << toJson(map).dump(kIndent) << '\n';
        out.flush();

        if (!out)
        {
            throw fileError("the map file cannot be written", path);
        }
    }

    namespace
    {
        [[nodiscard]] nlohmann::json parsedMapFile(
            const std::filesystem::path &path)
        {
            std::ifstream in(path);

            if (!in)
            {
                throw fileError(
                    "the map file cannot be opened for reading",
                    path);
            }

            nlohmann::json document;

            try
            {
                in >> document;
            }
            catch (const nlohmann::json::exception &error) // GCOVR_EXCL_LINE
            {
                throw TileMapError(
                    std::string("the map file is not valid json: ")
                    + error.what());
            }

            return document;
        } // GCOVR_EXCL_LINE

        void writeMapFile(
            const std::filesystem::path &path,
            const nlohmann::json &document)
        {
            std::ofstream out(path);

            if (!out)
            {
                throw fileError(
                    "the map file cannot be opened for writing",
                    path);
            }

            out << document.dump(kIndent) << '\n';
            out.flush();

            if (!out)
            {
                throw fileError(
                    "the map file cannot be written", path);
            }
        }
    }

    TileMap loadMapFile(const std::filesystem::path &path)
    {
        return tileMapFromJson(parsedMapFile(path));
    }

    void saveMapFile(
        const std::filesystem::path &path,
        const MapDocument &document)
    {
        writeMapFile(path, toJson(document));
    }

    MapDocument loadMapDocumentFile(const std::filesystem::path &path)
    {
        return mapDocumentFromJson(parsedMapFile(path));
    }

}
