#include "antwika/pathfinding/GridGraph.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <utility>

#include "antwika/pathfinding/PathfindingError.hpp"

namespace antwika::pathfinding
{

    namespace
    {

        // Row-major order, so a tie already breaks as NodeId does.
        constexpr std::array<GridCell, 4> kSteps{
            GridCell{.x = 0, .y = -1},
            GridCell{.x = -1, .y = 0},
            GridCell{.x = 1, .y = 0},
            GridCell{.x = 0, .y = 1},
        };

        [[nodiscard]] Cost distance(std::int32_t left, std::int32_t right)
        {
            return std::abs(
                static_cast<Cost>(left) - static_cast<Cost>(right));
        }

    } // namespace

    GridGraph::GridGraph(
        std::int32_t width, std::int32_t height, std::vector<bool> passable)
        : gridWidth(width),
          gridHeight(height),
          passableCells(std::move(passable))
    {
        if (gridWidth <= 0 || gridHeight <= 0)
        {
            throw PathfindingError(
                "pathfinding: a grid needs a positive width and height");
        }

        const std::size_t expected =
            static_cast<std::size_t>(gridWidth)
            * static_cast<std::size_t>(gridHeight);

        // nodeAt() computes y * width + x in int32 and cellOf() casts
        // an index back signed; a grid whose cell count leaves int32
        // would make both of those lies, so it is refused here and the
        // two casts become provably safe.
        if (expected > static_cast<std::size_t>(
                std::numeric_limits<std::int32_t>::max()))
        {
            throw PathfindingError(
                "pathfinding: the grid's cell count leaves the 32-bit "
                "indices its nodes are numbered in");
        }

        if (passableCells.size() != expected)
        {
            throw PathfindingError(
                "pathfinding: passability count does not match the grid");
        }
    }

    std::int32_t GridGraph::width() const noexcept
    {
        return gridWidth;
    }

    std::int32_t GridGraph::height() const noexcept
    {
        return gridHeight;
    }

    bool GridGraph::contains(GridCell cell) const noexcept
    {
        return cell.x >= 0 && cell.x < gridWidth && cell.y >= 0
            && cell.y < gridHeight;
    }

    bool GridGraph::passable(GridCell cell) const
    {
        return passableCells[rawValue(nodeAt(cell))];
    }

    NodeId GridGraph::nodeAt(GridCell cell) const
    {
        if (!contains(cell))
        {
            throw PathfindingError(
                "pathfinding: cell lies outside the grid");
        }

        return nodeId(
            static_cast<std::uint32_t>(cell.y * gridWidth + cell.x));
    }

    GridCell GridGraph::cellOf(NodeId node) const
    {
        const std::uint32_t index = rawValue(node);

        if (index >= passableCells.size())
        {
            throw PathfindingError(
                "pathfinding: node lies outside the grid");
        }

        const std::int32_t signedIndex = static_cast<std::int32_t>(index);

        return GridCell{
            .x = signedIndex % gridWidth,
            .y = signedIndex / gridWidth,
        };
    }

    void GridGraph::neighbours(
        NodeId from, std::vector<Neighbour> &out) const
    {
        const GridCell cell = cellOf(from);

        if (!passable(cell))
        {
            return;
        }

        for (const GridCell &step : kSteps)
        {
            const GridCell candidate{
                .x = cell.x + step.x,
                .y = cell.y + step.y,
            };

            if (!contains(candidate) || !passable(candidate))
            {
                continue;
            }

            out.push_back(Neighbour{
                .node = nodeAt(candidate),
                .cost = kGridStepCost,
            });
        }
    }

    Cost GridGraph::heuristic(NodeId from, NodeId goal) const
    {
        const GridCell here = cellOf(from);
        const GridCell there = cellOf(goal);

        return (distance(here.x, there.x) + distance(here.y, there.y))
            * kGridStepCost;
    }

} // namespace antwika::pathfinding
