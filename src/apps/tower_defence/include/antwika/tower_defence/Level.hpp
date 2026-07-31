#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "antwika/tower_defence/LevelTile.hpp"

namespace antwika::tower_defence
{

    /**
     * @brief A cell address in the level grid.
     *
     * Integers only, like everything else the simulation holds: a cell
     * is what a click resolves to and what a mob's position indexes.
     */
    struct Cell
    {
        std::uint32_t x = 0;
        std::uint32_t y = 0;

        [[nodiscard]] bool operator==(const Cell &) const = default;
    };

    /**
     * @brief A generated level: a grid of tiles plus the walk over them.
     *
     * `path` is the whole point of the type. It runs from the Start tile
     * to the End tile, one cell per step, and every non-Empty tile in
     * `tiles` appears in it exactly once -- so a mob's progress is a
     * single index into it rather than a search.
     */
    struct Level
    {
        std::uint32_t width = 0;
        std::uint32_t height = 0;

        /** @brief Row-major, width * height entries. */
        std::vector<Tile> tiles;

        /** @brief Start to end, in walk order. */
        std::vector<Cell> path;

        /**
         * @brief Read the tile at a cell.
         * @param cell The cell to read; must be inside the grid.
         * @return The tile there.
         */
        [[nodiscard]] Tile at(const Cell &cell) const
        {
            return tiles[
                static_cast<std::size_t>(cell.y) * width + cell.x];
        }
    };

} // namespace antwika::tower_defence
