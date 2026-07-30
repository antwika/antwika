#include "antwika/game/RoadGraph.hpp"

#include <array>
#include <cstdlib>

namespace antwika::game
{

    namespace
    {
        // The same four, in the same order, every expansion.
        constexpr std::array<Direction, kDirectionCount> kOutwards{
            Direction::North,
            Direction::East,
            Direction::South,
            Direction::West};

        // One step costs one, so a path's cost is its length.
        constexpr pathfinding::Cost kStepCost = 1;
    } // namespace

    RoadGraph::RoadGraph(const PathIndex &paths) : paths(paths)
    {
    }

    void RoadGraph::neighbours(
        pathfinding::NodeId from,
        std::vector<pathfinding::Neighbour> &out) const
    {
        const auto at = cellFor(from);

        for (const auto direction : kOutwards)
        {
            const auto onto = step(at, direction);
            if (!paths.has(onto))
            {
                continue;
            }

            out.push_back(
                pathfinding::Neighbour{
                    .node = nodeFor(onto), .cost = kStepCost});
        }
    }

    pathfinding::Cost RoadGraph::heuristic(
        pathfinding::NodeId from, pathfinding::NodeId goal) const
    {
        const auto here = cellFor(from);
        const auto there = cellFor(goal);

        return std::abs(static_cast<pathfinding::Cost>(here.x - there.x))
               + std::abs(
                   static_cast<pathfinding::Cost>(here.y - there.y));
    }

} // namespace antwika::game
