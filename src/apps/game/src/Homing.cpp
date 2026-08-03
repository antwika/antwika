#include "antwika/game/Homing.hpp"

#include <cstddef>
#include <cstdint>
#include <utility>
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

        [[nodiscard]] std::size_t indexOf(
            Cell cell, GridExtent extent) noexcept
        {
            return static_cast<std::size_t>(cell.y)
                       * static_cast<std::size_t>(extent.width)
                   + static_cast<std::size_t>(cell.x);
        }

        // The goal's own block, however passability was decided.
        // A building stands on no road and is not open ground either.
        // So without this exception nobody could ever arrive.
        void openGoal(
            std::vector<bool> &passable,
            GridExtent extent,
            Cell goal,
            Footprint footprint)
        {
            for (std::int32_t dy = 0; dy < footprint.height; ++dy)
            {
                for (std::int32_t dx = 0; dx < footprint.width; ++dx)
                {
                    const Cell on{.x = goal.x + dx, .y = goal.y + dy};

                    if (extent.contains(on))
                    {
                        passable[indexOf(on, extent)] = true;
                    }
                }
            }
        }

        // Every road, plus the goal.
        // Walked over the roads rather than over every cell.
        // A city is mostly not road, by a wide margin.
        [[nodiscard]] std::vector<bool> alongRoads(
            const PathIndex &paths,
            GridExtent extent,
            Cell goal,
            Footprint footprint)
        {
            std::vector<bool> passable(
                static_cast<std::size_t>(extent.width)
                    * static_cast<std::size_t>(extent.height),
                false);

            for (const auto road : paths.cells())
            {
                if (extent.contains(road))
                {
                    passable[indexOf(road, extent)] = true;
                }
            }

            openGoal(passable, extent, goal, footprint);

            return passable;
            // The excluded line is the vector's unwind destructor.
            // Nothing between its construction and the return throws.
            // An indexed write into a sized vector<bool> does not.
            // Nor does walking a std::set, nor openGoal().
        } // GCOVR_EXCL_LINE

        // Everything but a building, plus the goal.
        // Walked over the buildings for alongRoads()' reason exactly.
        // A city is mostly not built on either.
        [[nodiscard]] std::vector<bool> overOpenGround(
            const BuildingIndex &built,
            GridExtent extent,
            Cell goal,
            Footprint footprint)
        {
            std::vector<bool> passable(
                static_cast<std::size_t>(extent.width)
                    * static_cast<std::size_t>(extent.height),
                true);

            for (const auto cell : built.cells())
            {
                if (extent.contains(cell))
                {
                    passable[indexOf(cell, extent)] = false;
                }
            }

            openGoal(passable, extent, goal, footprint);

            return passable;
            // Excluded for alongRoads()' reason exactly.
        } // GCOVR_EXCL_LINE

        // The one search every answer below is read off.
        // Stated once so the graph is built one way.
        // Which is what keeps the tie-break the same for all four.
        // Cells rather than nodes.
        // A NodeId means nothing without the graph it came from.
        [[nodiscard]] std::vector<Cell> routeOver(
            Cell from,
            Cell goal,
            GridExtent extent,
            std::vector<bool> passable)
        {
            std::vector<Cell> route;

            // A degenerate extent holds nothing, so nothing is reachable.
            // Asked before GridGraph, which refuses a non-positive one.
            if (!extent.contains(from) || !extent.contains(goal))
            {
                return route;
            }

            const GridGraph graph(
                extent.width, extent.height, std::move(passable));

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

        [[nodiscard]] std::optional<Direction> firstStepOf(
            const std::vector<Cell> &route, Cell from)
        {
            // Already there, so there is no first step to report.
            if (route.size() < 2)
            {
                return std::nullopt;
            }

            const auto next = route[1];

            // Read off the delta rather than searched for among four.
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

        [[nodiscard]] std::optional<std::int64_t> lengthOf(
            const std::vector<Cell> &route)
        {
            // An empty route is no route.
            // A one-cell one is standing on it, which costs nothing.
            // Those are not the same answer.
            if (route.empty())
            {
                return std::nullopt;
            }

            return static_cast<std::int64_t>(route.size() - 1);
        }
    } // namespace

    std::optional<Direction> stepTowards(
        Cell from,
        Cell goal,
        Footprint footprint,
        const PathIndex &paths,
        GridExtent extent)
    {
        return firstStepOf(
            routeOver(
                from,
                goal,
                extent,
                alongRoads(paths, extent, goal, footprint)),
            from);
    }

    std::optional<std::int64_t> routeCost(
        Cell from,
        Cell goal,
        Footprint footprint,
        const PathIndex &paths,
        GridExtent extent)
    {
        return lengthOf(
            routeOver(
                from,
                goal,
                extent,
                alongRoads(paths, extent, goal, footprint)));
    }

    std::optional<Direction> stepAcross(
        Cell from,
        Cell goal,
        Footprint footprint,
        const BuildingIndex &built,
        GridExtent extent)
    {
        return firstStepOf(
            routeOver(
                from,
                goal,
                extent,
                overOpenGround(built, extent, goal, footprint)),
            from);
    }

    std::optional<std::int64_t> crossingCost(
        Cell from,
        Cell goal,
        Footprint footprint,
        const BuildingIndex &built,
        GridExtent extent)
    {
        return lengthOf(
            routeOver(
                from,
                goal,
                extent,
                overOpenGround(built, extent, goal, footprint)));
    }

} // namespace antwika::game
