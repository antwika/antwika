#pragma once

#include <cstdint>
#include <vector>

#include "antwika/pathfinding/Cost.hpp"
#include "antwika/pathfinding/IGraph.hpp"
#include "antwika/pathfinding/Neighbour.hpp"
#include "antwika/pathfinding/NodeId.hpp"

namespace antwika::pathfinding
{

    struct GridCell final
    {
        std::int32_t x;
        std::int32_t y;

        [[nodiscard]] bool operator==(const GridCell &other) const = default;
    };

    inline constexpr Cost kGridStepCost = 1;

    class GridGraph final : public IGraph
    {
    public:
        GridGraph(
            std::int32_t width,
            std::int32_t height,
            std::vector<bool> passable);

        [[nodiscard]] std::int32_t width() const noexcept;

        [[nodiscard]] std::int32_t height() const noexcept;

        [[nodiscard]] bool contains(GridCell cell) const noexcept;

        [[nodiscard]] bool passable(GridCell cell) const;

        [[nodiscard]] NodeId nodeAt(GridCell cell) const;

        [[nodiscard]] GridCell cellOf(NodeId node) const;

        void neighbours(
            NodeId from, std::vector<Neighbour> &out) const override;

        [[nodiscard]] Cost heuristic(NodeId from, NodeId goal) const override;

    private:
        std::int32_t gridWidth;
        std::int32_t gridHeight;
        std::vector<bool> passableCells;
    };

}
