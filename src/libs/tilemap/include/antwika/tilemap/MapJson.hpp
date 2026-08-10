#pragma once

#include <nlohmann/json_fwd.hpp>

#include <vector>

#include "antwika/tilemap/TileMap.hpp"

namespace antwika::tilemap
{

    struct MapDocument final
    {
        TileMap map;
        std::vector<bool> free{};
    };

    /**
     * @brief Serializes a map with an all-pinned free section.
     *
     * Ensures: the document always carries schema version 2 and an
     *          always-emitted "free" section of '.' rows.
     */
    [[nodiscard]] nlohmann::json toJson(const TileMap &map);

    /**
     * @brief Serializes a map beside its free mask.
     *
     * Ensures: cells beyond the mask's length serialize as pinned.
     */
    [[nodiscard]] nlohmann::json toJson(const MapDocument &document);

    /**
     * @brief Reads a map out of a map document.
     *
     * @param document The parsed map file.
     * @return The map it describes.
     * @throws TileMapError If the document fails validation or names
     *                      a schema version this build does not know.
     *
     * Ensures: version 1 and version 2 documents both load, and the
     *          free section is ignored here.
     */
    [[nodiscard]] TileMap tileMapFromJson(
        const nlohmann::json &document);

    /**
     * @brief Reads a map and its free mask out of a map document.
     *
     * @throws TileMapError If validation fails, a version 2 document
     *                      lacks the "free" section or its rows do
     *                      not match the grid, or the version is
     *                      unknown.
     *
     * Ensures: a version 1 document loads with an all-pinned mask.
     */
    [[nodiscard]] MapDocument mapDocumentFromJson(
        const nlohmann::json &document);

}
