#pragma once

#include <cstdint>
#include <vector>

#include "antwika/life/Grid.hpp"

namespace antwika::life
{

    /**
     * @brief Plain snapshot of every cell's alive state at one instant.
     *
     * World holds live entities and can't be copied or compared, so this
     * is a read-only copy taken from it instead -- the same role
     * apps/game's GameState plays for its own reducer-based state.
     */
    struct Board
    {
        std::uint32_t width{};
        std::uint32_t height{};
        std::vector<bool> alive;

        bool operator==(const Board &other) const = default;
    };

    /**
     * @brief Read every cell's current alive state out of world.
     * @param world World to read from, as of its last commit().
     * @param grid Maps the board's (x, y) coordinates to entities.
     * @return A Board snapshot, row-major, matching grid's dimensions.
     */
    [[nodiscard]] Board readBoard(const World &world, const Grid &grid);

} // namespace antwika::life
