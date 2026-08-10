#pragma once

#include <string>
#include <utility>
#include <vector>

#include <antwika/geometry/Grid.hpp>
#include <antwika/tilemap/TileMap.hpp>

#include "antwika/mapcheck/Finding.hpp"
#include "antwika/mapcheck/MapReport.hpp"

namespace antwika::mapcheck
{

    /**
     * @brief Validates one map's traversal from an entry cell.
     *
     * @param map The map to validate.
     * @param entry The cell traversal starts from.
     * @param grantedTags The tags held before the map grants any.
     * @return The findings and the fixed-point reachability from entry.
     *
     * Ensures: every finding's map field is left empty for the caller.
     * Ensures: an entry outside the grid is a finding, not a throw.
     */
    [[nodiscard]] MapReport validateMap(
        const tilemap::TileMap &map,
        geometry::GridCell entry,
        const std::vector<std::string> &grantedTags);

    /**
     * @brief Validates transition counterparts across a set of maps.
     *
     * @param maps The maps to validate, each paired with its name.
     * @return One finding per violation, naming the owning map.
     */
    [[nodiscard]] std::vector<Finding> validateWorld(
        const std::vector<std::pair<std::string, tilemap::TileMap>> &maps);

}
