#include "antwika/game/Homing.hpp"

#include <cstddef>
#include <cstdint>
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
            const PathIndex &paths,
            GridExtent extent,
            Cell goal,
            Footprint footprint)
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

            for (std::int32_t dy = 0; dy < footprint.height; ++dy)
            {
                for (std::int32_t dx = 0; dx < footprint.width; ++dx)
                {
                    const Cell on{.x = goal.x + dx, .y = goal.y + dy};

                    if (!extent.contains(on))
                    {
                        continue;
                    }

                    passable[static_cast<std::size_t>(on.y) * width
                             + static_cast<std::size_t>(on.x)] = true;
                }
            }

            return passable;
        }

        // The one search both answers below are read off.
        // Stated once so the graph is built one way.
        // Which is what keeps the tie-break the same for both.
        // Cells rather than nodes.
        // A NodeId means nothing without the graph it came from.
        [[nodiscard]] std::vector<Cell> routeOver(
            Cell from,
            Cell goal,
            Footprint footprint,
            const PathIndex &paths,
            GridExtent extent)
        {
            std::vector<Cell> route;

            // A degenerate extent holds nothing, so nothing is reachable.
            // Asked before GridGraph, which refuses a non-positive one.
            if (!extent.contains(from) || !extent.contains(goal))
            {
                return route;
            }

            const GridGraph graph(
                extent.width,
                extent.height,
                passableOver(paths, extent, goal, footprint));

            const auto result = antwika::pathfinding::findPath(
                graph, graph.nodeAt(asGridCell(from)),
                graph.nodeAt(asGridCell(goal)));

            if (result.outcome != SearchOutcome::PathFound)
            {
                return route;
            }

            route.reserve(result.nodes.size());

            for (const auto node : result.nodes)
            {
                const auto cell = graph.cellOf(node);
                route.push_back(Cell{.x = cell.x, .y = cell.y});
            }

            return route;
            // The excluded line is the local route's unwind destructor.
            // Nothing between its construction and the return throws.
        } // GCOVR_EXCL_LINE
    } // namespace

    std::optional<Direction> stepTowards(
        Cell from,
        Cell goal,
        Footprint footprint,
        const PathIndex &paths,
        GridExtent extent)
    {
        const auto route =
            routeOver(from, goal, footprint, paths, extent);

        // Already there, so there is no first step to report.
        if (route.size() < 2)
        {
            return std::nullopt;
        }

        const auto next = route[1];

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

    std::optional<std::int64_t> routeCost(
        Cell from,
        Cell goal,
        Footprint footprint,
        const PathIndex &paths,
        GridExtent extent)
    {
        const auto route =
            routeOver(from, goal, footprint, paths, extent);

        // An empty route is no route; a one-cell one is standing on it.
        // Which costs nothing to reach and is not the same answer.
        if (route.empty())
        {
            return std::nullopt;
        }

        return static_cast<std::int64_t>(route.size() - 1);
    }

} // namespace antwika::game
