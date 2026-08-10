#pragma once

#include <filesystem>

#include "antwika/tilemap/MapJson.hpp"
#include "antwika/tilemap/TileMap.hpp"

namespace antwika::tilemap
{

    /**
     * @brief Writes a map to a map file.
     *
     * @param path The file to write.
     * @param map The map to write into it.
     * @throws TileMapError If the file cannot be opened or written.
     */
    void saveMapFile(
        const std::filesystem::path &path, const TileMap &map);

    /**
     * @brief Reads a map from a map file.
     *
     * @param path The file to read.
     * @return The map it holds.
     * @throws TileMapError If the file cannot be read, is not valid
     *                      json, fails validation, or names a schema
     *                      version this build does not know.
     */
    [[nodiscard]] TileMap loadMapFile(
        const std::filesystem::path &path);

    /**
     * @brief Writes a map beside its free mask to a map file.
     *
     * @throws TileMapError If the file cannot be opened or written.
     */
    void saveMapFile(
        const std::filesystem::path &path,
        const MapDocument &document);

    /**
     * @brief Reads a map and its free mask from a map file.
     *
     * @throws TileMapError If the file cannot be read, is not valid
     *                      json, fails validation, or names a schema
     *                      version this build does not know.
     *
     * Ensures: a version 1 file loads with an all-pinned mask.
     */
    [[nodiscard]] MapDocument loadMapDocumentFile(
        const std::filesystem::path &path);

}
