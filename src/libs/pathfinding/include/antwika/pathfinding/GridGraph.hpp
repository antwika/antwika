#pragma once

#include <cstdint>
#include <vector>

#include "antwika/pathfinding/Cost.hpp"
#include "antwika/pathfinding/IGraph.hpp"
#include "antwika/pathfinding/Neighbour.hpp"
#include "antwika/pathfinding/NodeId.hpp"

namespace antwika::pathfinding
{

    /**
     * @brief A cell of a GridGraph, counted from its top-left corner.
     */
    struct GridCell
    {
        std::int32_t x;
        std::int32_t y;

        /**
         * @brief Compare two cells componentwise.
         * @param other The cell to compare against.
         * @return True iff both coordinates match.
         */
        [[nodiscard]] bool operator==(const GridCell &other) const = default;
    };

    /**
     * @brief What one step between orthogonally adjacent cells costs.
     *
     * Every step costs the same, which is what makes the Manhattan
     * distance an exact lower bound rather than merely a plausible one.
     */
    inline constexpr Cost kGridStepCost = 1;

    /**
     * @brief A 4-connected grid of passable and impassable cells, as an
     * IGraph.
     *
     * This is a convenience for the commonest caller, layered on the
     * generic search rather than built into it -- the search still has
     * no idea a grid exists, and an application with a different world
     * implements IGraph itself instead of bending its world into this
     * one.
     *
     * A cell is addressed by a NodeId holding its row-major index, so a
     * caller that already numbers its own cells that way needs no
     * translation table. Ties in the search break on ascending NodeId,
     * which here means a route through a cell earlier in row-major
     * order wins -- upwards and leftwards, in the picture.
     */
    class GridGraph final : public IGraph
    {
    public:
        /**
         * @brief Build a grid of a given extent and passability.
         * @param width Cells across, which must be positive.
         * @param height Cells down, which must be positive.
         * @param passable One flag per cell in row-major order, true
         * where a route may pass.
         * @throws PathfindingError If either extent is not positive, or
         * the flag count is not width * height.
         */
        GridGraph(
            std::int32_t width,
            std::int32_t height,
            std::vector<bool> passable);

        /**
         * @brief Cells across.
         * @return The width the grid was built with.
         */
        [[nodiscard]] std::int32_t width() const noexcept;

        /**
         * @brief Cells down.
         * @return The height the grid was built with.
         */
        [[nodiscard]] std::int32_t height() const noexcept;

        /**
         * @brief Whether the grid holds a cell at all.
         * @param cell The cell to test.
         * @return True iff both coordinates are inside the extent.
         */
        [[nodiscard]] bool contains(GridCell cell) const noexcept;

        /**
         * @brief Whether a route may pass through a cell.
         * @param cell The cell to test.
         * @return Its passability flag.
         * @throws PathfindingError If the cell is outside the grid.
         */
        [[nodiscard]] bool passable(GridCell cell) const;

        /**
         * @brief The node naming a cell.
         * @param cell The cell to name.
         * @return Its row-major index as a NodeId.
         * @throws PathfindingError If the cell is outside the grid.
         */
        [[nodiscard]] NodeId nodeAt(GridCell cell) const;

        /**
         * @brief The cell a node names.
         * @param node The node to place.
         * @return The cell at that row-major index.
         * @throws PathfindingError If the node is outside the grid.
         */
        [[nodiscard]] GridCell cellOf(NodeId node) const;

        /**
         * @brief Append the passable orthogonal neighbours of a cell.
         * @param from The node being expanded.
         * @param out The vector to append to.
         * @throws PathfindingError If the node is outside the grid.
         * @note An impassable cell has no neighbours, so a search that
         * starts or ends on one reports NoPath rather than throwing --
         * except where the start already is the goal, which findPath()
         * answers before it asks the graph anything.
         */
        void neighbours(
            NodeId from, std::vector<Neighbour> &out) const override;

        /**
         * @brief The Manhattan distance between two cells, in cost.
         * @param from The node being estimated.
         * @param goal The goal the search is heading for.
         * @return The step count of the shortest obstacle-free route,
         * times kGridStepCost.
         * @throws PathfindingError If either node is outside the grid.
         * @note Consistent, not merely admissible: one step changes the
         * distance by exactly one, never more, which is what IGraph
         * asks for.
         */
        [[nodiscard]] Cost heuristic(NodeId from, NodeId goal) const override;

    private:
        std::int32_t gridWidth;
        std::int32_t gridHeight;
        std::vector<bool> passableCells;
    };

} // namespace antwika::pathfinding
