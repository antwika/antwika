#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "antwika/tower_defence/LevelTile.hpp"

namespace antwika::tower_defence
{

    struct Cell final
    {
        std::uint32_t x = 0;
        std::uint32_t y = 0;

        [[nodiscard]] bool operator==(const Cell &) const = default;
    };

    struct Level final
    {
        std::uint32_t width = 0;
        std::uint32_t height = 0;

        std::vector<Tile> tiles;

        std::vector<Cell> path;

        [[nodiscard]] Tile at(const Cell &cell) const
        {
            return tiles[
                static_cast<std::size_t>(cell.y) * width + cell.x];
        }
    };

}
