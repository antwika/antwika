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

    /**
     * @brief Read every cell's current alive state out of world, without
     * a Grid to map coordinates with.
     *
     * For observers constructed before bootstrap() creates the Grid, so
     * they cannot hold a reference to one -- the same position PrintSystem
     * is in. It relies instead on Grid creating cells in row-major order
     * and ComponentStorage's insertion-order stability (see antwika::ecs)
     * to read world.view<Cell>() back out in that same row-major order.
     * Prefer readBoard() wherever a Grid is available: it maps every
     * coordinate explicitly and so depends on no such convention.
     *
     * Cells beyond width * height are ignored, and dimensions the world
     * has no cell for are reported as dead, so the returned Board always
     * has exactly width * height entries.
     *
     * @param world World to read from, as of its last commit().
     * @param width Number of columns to read.
     * @param height Number of rows to read.
     * @return A Board snapshot, row-major, of the requested dimensions.
     */
    [[nodiscard]] Board readBoardFromView(
        const World &world, std::uint32_t width, std::uint32_t height);

} // namespace antwika::life
