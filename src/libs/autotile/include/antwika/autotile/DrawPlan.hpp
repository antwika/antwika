#pragma once

#include <cstdint>

#include <antwika/geometry/Grid.hpp>
#include <antwika/tilemap/TileMap.hpp>

#include "antwika/autotile/Connectors.hpp"
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

    /**
     * @brief Builds the plan with an edge-aware variant scatter.
     *
     * @param connectors Per terrain and variant, which display-tile
     *        edges carry a connector; the base slot is index zero.
     *
     * Ensures: interior variants are chosen row-major so every
     *          choice matches the west and north neighbours' facing
     *          edges, connector to connector and blank to blank;
     *          when the status-quo pick fails, the fallback prefers
     *          candidates matching both edges (base first, else by
     *          position hash), then west-only, then north-only,
     *          then the base.
     * Ensures: all-connected connectors reproduce the plain
     *          overload's plan exactly.
     */
    [[nodiscard]] DrawPlan buildDrawPlan(
        const tilemap::TileMap &map,
        geometry::GridCell player,
        std::int32_t playerHeight,
        std::uint32_t clock,
        const TerrainConnectors &connectors);

}
