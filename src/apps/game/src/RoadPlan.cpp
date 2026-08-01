#include "antwika/game/RoadPlan.hpp"

#include <cstddef>
#include <cstdint>

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

        // Everything but a building, which a road may not cross.
        // Walked over the buildings rather than over every cell.
        // A city is mostly not built on, by a wide margin.
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
    } // namespace

    RoadPlan planRoad(
        Cell from, Cell to, GridExtent extent, const BuildingIndex &built)
    {
        RoadPlan plan;

        // A degenerate extent holds nothing, so nothing is reachable.
        // Asked before GridGraph, which refuses a non-positive extent.
        // A cell off the grid is not a cell somebody could have meant.
        // So there is nothing to refuse in the picture either.
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

        // No route is an ordinary answer rather than a failure.
        // The product decision here is the simple honest one.
        // Build nothing at all.
        // And show the two cells that were named as refused.
        // Laying the part that did fit would put down a road to nowhere.
        if (result.outcome != SearchOutcome::PathFound)
        {
            // Both, unconditionally.
            // A start that is already the goal is never refused.
            // So a cell can never fail to reach itself here.
            // Which is why there is no one-cell refusal to word.
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

} // namespace antwika::game
