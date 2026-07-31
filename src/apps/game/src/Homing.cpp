#include "antwika/game/Homing.hpp"

#include <cstddef>
#include <vector>

#include <antwika/pathfinding/AStar.hpp>
#include <antwika/pathfinding/GridGraph.hpp>
#include <antwika/pathfinding/SearchResult.hpp>

#include "antwika/game/Direction.hpp"

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

        // Every road is passable, plus the goal itself.
        // A building does not stand on a road.
        // So without that exception nobody could ever arrive.
        [[nodiscard]] std::vector<bool> passableOver(
            const PathIndex &paths, GridExtent extent, Cell goal)
        {
            const auto width = static_cast<std::size_t>(extent.width);
            const auto height = static_cast<std::size_t>(extent.height);

            std::vector<bool> passable(width * height, false);

            // Walked over the roads rather than over every cell.
            // A city is mostly not road, by a wide margin.
            for (const auto road : paths.cells())
            {
                if (!extent.contains(road))
                {
                    continue;
                }

                passable[static_cast<std::size_t>(road.y) * width
                         + static_cast<std::size_t>(road.x)] = true;
            }

            passable[static_cast<std::size_t>(goal.y) * width
                     + static_cast<std::size_t>(goal.x)] = true;

            return passable;
        }
    } // namespace

    std::optional<Direction> stepTowards(
        Cell from, Cell goal, const PathIndex &paths, GridExtent extent)
    {
        // A degenerate extent holds nothing, so nothing is reachable.
        // Asked before GridGraph, which refuses a non-positive extent.
        if (!extent.contains(from) || !extent.contains(goal))
        {
            return std::nullopt;
        }

        const GridGraph graph(
            extent.width, extent.height, passableOver(paths, extent, goal));

        const auto result = antwika::pathfinding::findPath(
            graph, graph.nodeAt(asGridCell(from)),
            graph.nodeAt(asGridCell(goal)));

        // Already there, so there is no first step to report.
        if (result.outcome != SearchOutcome::PathFound
            || result.nodes.size() < 2)
        {
            return std::nullopt;
        }

        const auto next = graph.cellOf(result.nodes[1]);

        // Read off the delta rather than searched for among the four.
        // A GridGraph neighbour is orthogonal by construction.
        // So exactly one of these holds.
        // A search would instead have an exit nothing could reach.
        if (next.x > from.x)
        {
            return Direction::East;
        }

        if (next.x < from.x)
        {
            return Direction::West;
        }

        if (next.y > from.y)
        {
            return Direction::South;
        }

        return Direction::North;
    }

} // namespace antwika::game
