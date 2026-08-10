#pragma once

#include <cstdint>
#include <vector>

#include <antwika/geometry/Grid.hpp>
#include <antwika/tilemap/TileMap.hpp>

namespace antwika::autotile
{

    /**
     * @brief Marks the block of high ground that occludes the player.
     *
     * @return One flag per cell, indexed row by row; a set flag means
     *         the cell hides its parts above the player's height.
     */
    [[nodiscard]] std::vector<bool> cutawayHidden(
        const tilemap::TileMap &map,
        geometry::GridCell player,
        std::int32_t playerHeight);

}
