#pragma once

#include <cstdint>
#include <vector>

#include <antwika/pathfinding/Cost.hpp>
#include <antwika/pathfinding/IGraph.hpp>
#include <antwika/pathfinding/Neighbour.hpp>
#include <antwika/pathfinding/NodeId.hpp>

#include "antwika/game/Cell.hpp"
#include "antwika/game/Direction.hpp"
#include "antwika/game/PathIndex.hpp"

namespace antwika::game
{

    /**
     * @brief Half the window a cell coordinate is numbered over.
     *
     * A NodeId is 32 bits and a Cell is two signed 32-bit numbers, so
     * the numbering has to pick a window.  Sixteen bits per axis,
     * biased by this, is what it picks: every cell a GridExtent can
     * hold is inside it many times over, since an extent starts at the
     * origin and is measured in tiles somebody has to click on.
     */
    inline constexpr std::int32_t kCellNumberBias = 0x8000;

    /**
     * @brief Number a cell, for the pathfinding library.
     *
     * The numbering is a pure function of the cell and of nothing else
     * -- not of which cells have roads, not of the order they were
     * laid in -- which is what makes a search over it replay.  The
     * library's last tie-break is ascending NodeId, so a numbering
     * that shifted as roads were laid would quietly hand back a
     * different, equally cheap route on the second run.
     *
     * It is also *order-preserving*: x lands in the high half and y in
     * the low one, so ascending NodeId is exactly the ascending order
     * Cell already defines.  The tie-break therefore reads as "the
     * lowest cell wins" rather than as an implementation detail.
     *
     * @param cell The cell to number; both coordinates must lie in
     * [-kCellNumberBias, kCellNumberBias), which every cell inside a
     * GridExtent does.
     * @return The node that names it.
     */
    [[nodiscard]] constexpr pathfinding::NodeId nodeFor(Cell cell) noexcept
    {
        const auto x = static_cast<std::uint32_t>(cell.x + kCellNumberBias);
        const auto y = static_cast<std::uint32_t>(cell.y + kCellNumberBias);

        return pathfinding::nodeId((x << 16U) | y);
    }

    /**
     * @brief Get back the cell a node names.
     * @param node The node to read; one nodeFor() produced.
     * @return The cell it stands for.
     */
    [[nodiscard]] constexpr Cell cellFor(pathfinding::NodeId node) noexcept
    {
        const auto raw = pathfinding::rawValue(node);

        return Cell{
            .x = static_cast<std::int32_t>(raw >> 16U) - kCellNumberBias,
            .y = static_cast<std::int32_t>(raw & 0xFFFFU)
                 - kCellNumberBias};
    }

    /**
     * @brief Get the direction one step takes from one cell to the next.
     * @param from The cell being left.
     * @param to The cell being entered; orthogonally adjacent to it.
     * @return The direction of that step.
     */
    [[nodiscard]] constexpr Direction headingTo(Cell from, Cell to) noexcept
    {
        if (to.y != from.y)
        {
            return to.y < from.y ? Direction::North : Direction::South;
        }

        return to.x < from.x ? Direction::West : Direction::East;
    }

    /**
     * @brief The road network, as the pathfinding library sees it.
     *
     * An adapter in the app rather than a graph in the library, for the
     * reason IGraph gives for having no cell type of its own: the roads
     * are already an ordered index, so answering "which way out of
     * here" is four PathIndex lookups and needs no second copy of the
     * board.  GridGraph would want a rectangle of costs the size of the
     * extent, most of it impassable.
     *
     * Only cells with a road on them are nodes.  A building's own cell
     * is therefore *not* in this graph, which is why a walker's origin
     * is the road it stepped out onto rather than the building behind
     * it -- a route home ends at the door.
     */
    class RoadGraph final : public pathfinding::IGraph
    {
    public:
        /**
         * @brief Construct the graph over the roads it describes.
         * @param paths The roads; must outlive this graph, and a search
         * must not run while they are being changed.
         */
        explicit RoadGraph(const PathIndex &paths);

        /**
         * @brief Append the roads leading out of a cell.
         *
         * North, east, south and west, always in that order, each
         * costing one step.  The order does not decide anything -- the
         * library's tie-break is total without it -- but a fixed one
         * costs nothing and keeps the graph as reproducible as the
         * search reading it.
         *
         * @param from The node being expanded.
         * @param out The vector to append to.
         */
        void neighbours(
            pathfinding::NodeId from,
            std::vector<pathfinding::Neighbour> &out) const override;

        /**
         * @brief Estimate the steps left to the goal.
         * @param from The node being estimated.
         * @param goal The node the search is heading for.
         * @return The Manhattan distance between the two cells, which
         * is consistent for a 4-connected graph of unit-cost edges: one
         * step changes it by exactly one.
         */
        [[nodiscard]] pathfinding::Cost heuristic(
            pathfinding::NodeId from,
            pathfinding::NodeId goal) const override;

    private:
        const PathIndex &paths;
    };

} // namespace antwika::game
