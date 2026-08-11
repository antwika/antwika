#pragma once

#include <cstdint>
#include <vector>

#include <antwika/geometry/Grid.hpp>
#include <antwika/tilemap/TileMap.hpp>

namespace antwika::autotile
{

    /**
     * @brief Marks the columns whose high slabs occlude the player.
     *
     * @return One flag per cell, indexed row by row; a set flag means
     *         the cell hides the parts of its column above the
     *         player's level.
     *
     * Ensures: the player's own column seeds the flood, so an
     *          overhang above the player opens up.
     */
    [[nodiscard]] std::vector<bool> cutawayHidden(
        const tilemap::TileMap &map,
        geometry::GridCell player,
        std::int32_t playerHeight);

}
