#pragma once

#include <array>
#include <cstdint>

#include <antwika/enums/Enumeration.hpp>
#include <antwika/geometry/Grid.hpp>
#include <antwika/tilemap/TerrainClass.hpp>
#include <antwika/tilemap/TileMap.hpp>
#include <antwika/tileset/Tileset.hpp>

#include "antwika/autotile/TileDraw.hpp"

namespace antwika::autotile
{

    struct TilesetBindings final
    {
        /**
         * @brief One tileset per terrain class, by enums::index.
         *
         * Requires: every entry is non-null before the bindings
         *           reach buildDrawPlan.
         */
        std::array<
            const tileset::Tileset *,
            enums::kCount<tilemap::TerrainClass>>
            byTerrain{};

        [[nodiscard]] bool operator==(
            const TilesetBindings &other) const = default;
    };

    /**
     * @brief Turns a semantic map into an ordered list of draws.
     *
     * @param map The semantic map to draw.
     * @param player The cell the player stands in.
     * @param playerHeight The level of the slab the player stands on.
     * @param clock The global animation clock, in frames.
     * @param bindings The tileset each terrain class assembles from.
     * @return The ordered draw plan.
     *
     * Requires: every pointer in bindings.byTerrain is non-null.
     * Ensures: entries are ordered back to front, so drawing them in
     *          sequence paints the map correctly.
     * Ensures: the same inputs yield the same plan on every run.
     * Ensures: a slab surfaces only while no slab sits on the level
     *          directly above it in its column, and every 8px lattice
     *          cell of a level's exposed region of one terrain whose
     *          base layer has sprites gets exactly one base draw,
     *          chosen so edge sockets face outside the region and
     *          adjoining sockets match west and north neighbours
     *          where the declared sprites allow it.
     * Ensures: wherever several sprites remain valid at a cell, the
     *          pick among them is deterministic and weighted, so a
     *          sprite's share of such cells is proportional to its
     *          weight.
     * Ensures: decor draws land only on lattice cells with a base
     *          draw, only on bases their sprite's on list names, and
     *          after the base draws of their terrain.
     * Ensures: Shade entries carry the shaded slab's terrain, and the
     *          renderer tints them black.
     */
    [[nodiscard]] DrawPlan buildDrawPlan(
        const tilemap::TileMap &map,
        geometry::GridCell player,
        std::int32_t playerHeight,
        std::uint32_t clock,
        const TilesetBindings &bindings);

}
