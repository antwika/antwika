#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include <antwika/log/ILogger.hpp>
#include <antwika/tilemap/TerrainClass.hpp>

#include "antwika/map_editor/EditorState.hpp"

namespace antwika::map_editor
{

    /**
     * @brief Solves terrains for the unpinned cells of the map.
     *
     * @param seed Varies which free cells get pre-collapsed.
     * @return One terrain per cell in row-major order, or nothing
     *         when the constraints admit no solution in budget.
     *
     * Ensures: pinned cells keep their current terrain in the
     *          returned assignment, and Stair never appears on a
     *          free cell.
     */
    [[nodiscard]] std::optional<std::vector<tilemap::TerrainClass>>
    generateTerrains(const EditorState &state, std::uint32_t seed);

    /**
     * @brief Runs one Generate action end to end.
     *
     * Ensures: the seed increments exactly once, a success applies
     *          one undoable edit, and a failure leaves the map
     *          untouched and raises the failure notice.
     */
    void generate(EditorState &state, log::ILogger &logger);

}
