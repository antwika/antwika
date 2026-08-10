#pragma once

#include <cstdint>

#include "antwika/tilemap/TileMap.hpp"

namespace antwika::tilemap
{

    /**
     * @brief Returns a larger map with the original in its interior.
     *
     * @param west Columns added before the first column.
     * @param north Rows added before the first row.
     * @param east Columns added after the last column.
     * @param south Rows added after the last row.
     * @return The grown map with cells copied at the offset, every
     *         entity origin shifted by (west, north), and the header
     *         preserved.
     *
     * Ensures: zero growth on every side returns an equal map, and
     *          the added cells hold default values.
     */
    [[nodiscard]] TileMap expandedMap(
        const TileMap &map,
        std::uint32_t west,
        std::uint32_t north,
        std::uint32_t east,
        std::uint32_t south);

}
