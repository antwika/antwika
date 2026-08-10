#pragma once

#include <cstdint>

#include <antwika/geometry/Grid.hpp>
#include <antwika/tilemap/TileMap.hpp>

#include "antwika/autotile/TileDraw.hpp"

namespace antwika::autotile
{

    /**
     * @brief Turns a semantic map into an ordered list of sprite draws.
     *
     * @param map The semantic map to draw.
     * @param player The cell the player stands in.
     * @param playerHeight The height level the player stands at.
     * @param clock The global animation clock, in frames.
     * @return The ordered draw plan.
     *
     * Ensures: entries are ordered back to front, so drawing them in
     * sequence paints the map correctly.
     * Ensures: the same inputs yield the same plan on every run.
     * Ensures: Shade entries carry the shaded cell's terrain, and the
     * renderer tints them black.
     */
    [[nodiscard]] DrawPlan buildDrawPlan(
        const tilemap::TileMap &map,
        geometry::GridCell player,
        std::int32_t playerHeight,
        std::uint32_t clock);

}
