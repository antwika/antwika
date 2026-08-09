#include "antwika/game/RoadPlan.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>

#include <antwika/pathfinding/AStar.hpp>
#include <antwika/pathfinding/GridGraph.hpp>
#include <antwika/pathfinding/SearchResult.hpp>

namespace antwika::game
{

    namespace
    {
        using antwika::pathfinding::GridCell;
        using antwika::pathfinding::GridGraph;
        using antwika::pathfinding::SearchOutcome;

        [[nodiscard]] GridCell asGridCell(Cell cell) noexcept
        {
            return GridCell{.x = cell.x, .y = cell.y};
        }

        [[nodiscard]] std::vector<bool> openOver(
            GridExtent extent, const BuildingIndex &built)
        {
            const auto width = static_cast<std::size_t>(extent.width);
            const auto height = static_cast<std::size_t>(extent.height);

            std::vector<bool> passable(width * height, true);

            for (const auto cell : built.cells())
            {
                if (!extent.contains(cell))
                {
                    continue;
                }

                passable[static_cast<std::size_t>(cell.y) * width
                         + static_cast<std::size_t>(cell.x)] = false;
            }

            return passable;
        }
    }

    RoadPlan planRoad(
        Cell from, Cell to, GridExtent extent, const BuildingIndex &built)
    {
        RoadPlan plan;

        if (!extent.contains(from) || !extent.contains(to))
        {
            return plan;
        }

        const GridGraph graph(
            extent.width, extent.height, openOver(extent, built));

        const auto result = antwika::pathfinding::findPath(
            graph,
            graph.nodeAt(asGridCell(from)),
            graph.nodeAt(asGridCell(to)));

        if (result.outcome != SearchOutcome::PathFound)
        {
            plan.cells.push_back(from);
            plan.cells.push_back(to);

            return plan;
        }

        plan.cells.reserve(result.nodes.size());

        for (const auto node : result.nodes)
        {
            const auto cell = graph.cellOf(node);
            plan.cells.push_back(Cell{.x = cell.x, .y = cell.y});
        }

        plan.valid = true;

        return plan;
    }

    RoadPlan planPlots(
        const Cell from, const Cell to, const GridExtent extent)
    {
        RoadPlan plan;

        if (!extent.contains(from) || !extent.contains(to))
        {
            return plan;
        }

        const auto left = std::min(from.x, to.x);
        const auto right = std::max(from.x, to.x);
        const auto top = std::min(from.y, to.y);
        const auto bottom = std::max(from.y, to.y);

        for (auto y = top; y <= bottom; ++y)
        {
            for (auto x = left; x <= right; ++x)
            {
                plan.cells.push_back(Cell{.x = x, .y = y});
            }
        }

        plan.valid = true;

        return plan;
    } // GCOVR_EXCL_LINE

    RoadPlan planDrag(
        const std::optional<BuildTool> tool,
        const Cell from,
        const Cell to,
        const GridExtent extent,
        const BuildingIndex &built)
    {
        if (!tool.has_value() || !dragsOut(*tool))
        {
            return RoadPlan{};
        }

        if (*tool == BuildTool::Road)
        {
            return planRoad(from, to, extent, built);
        }

        return planPlots(from, to, extent);
    }

}
