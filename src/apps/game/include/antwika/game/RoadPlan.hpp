#pragma once

#include <optional>
#include <vector>

#include "antwika/game/BuildingIndex.hpp"
#include "antwika/game/BuildTool.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/GridExtent.hpp"

namespace antwika::game
{

    struct RoadPlan final
    {
        std::vector<Cell> cells;

        bool valid = false;

        [[nodiscard]] bool operator==(const RoadPlan &other) const = default;
    };

    [[nodiscard]] RoadPlan planRoad(
        Cell from, Cell to, GridExtent extent, const BuildingIndex &built);

    /**
     * @brief Boxes in the plots a drag between two cells covers.
     *
     * @param from The cell the drag began on.
     * @param to The cell the drag reached.
     * @param extent The grid, outside which there is no plan at all.
     * @return The cells of the box, in reading order.
     */
    [[nodiscard]] RoadPlan planPlots(
        Cell from, Cell to, GridExtent extent);

    /**
     * @brief Plans what a drag with the tool in hand would lay down.
     *
     * @param tool The tool being dragged, if any.
     * @param from The cell the drag began on.
     * @param to The cell the drag reached.
     * @param extent The grid the drag runs over.
     * @param built What already stands, which a road plans around.
     * @return The plan, empty for a tool that does not drag.
     */
    [[nodiscard]] RoadPlan planDrag(
        std::optional<BuildTool> tool,
        Cell from,
        Cell to,
        GridExtent extent,
        const BuildingIndex &built);

}
