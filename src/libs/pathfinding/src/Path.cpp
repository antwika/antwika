#include "antwika/pathfinding/Path.hpp"

#include <algorithm>
#include <cstdlib>
#include <map>
#include <queue>
#include <utility>

namespace antwika::pathfinding
{

    namespace
    {
        [[nodiscard]] std::uint64_t getApart(
            const GridPos fromPosition, const GridPos toPosition)
        {
            return static_cast<std::uint64_t>(
                       std::abs(fromPosition.x - toPosition.x))
                   + static_cast<std::uint64_t>(
                       std::abs(fromPosition.z - toPosition.z));
        }

        [[nodiscard]] std::vector<GridPos> getWalkedBack(
            const std::map<GridPos, GridPos> &cameFromPos,
            const GridPos fromPosition,
            const GridPos toPosition)
        {
            std::vector<GridPos> gridPositions{toPosition};

            while (gridPositions.back() != fromPosition)
            {
                gridPositions.push_back(cameFromPos.at(gridPositions.back()));
            }

            std::reverse(gridPositions.begin(), gridPositions.end());

            return gridPositions;
        } // GCOVR_EXCL_LINE
    }

    std::optional<std::vector<GridPos>> getPathBetween(
        const IWalkGraph &graph,
        const GridPos fromPosition,
        const GridPos toPosition,
        const std::uint64_t maxNodesExplored)
    {
        if (fromPosition == toPosition)
        {
            return std::vector<GridPos>{fromPosition};
        }

        using Entry = std::pair<std::uint64_t, GridPos>;

        std::map<GridPos, GridPos> cameFromPos;
        std::map<GridPos, std::uint64_t> costTo;
        std::priority_queue<
            Entry,
            std::vector<Entry>,
            std::greater<>>
            open;

        costTo[fromPosition] = 0;
        open.push(Entry{getApart(fromPosition, toPosition), fromPosition});

        std::uint64_t takenSteps = 0;

        while (!open.empty())
        {
            if (++takenSteps > maxNodesExplored)
            {
                return std::nullopt;
            }

            const auto [worth, herePos] = open.top();

            open.pop();

            if (herePos == toPosition)
            {
                return getWalkedBack(cameFromPos, fromPosition, toPosition);
            }

            if (worth > costTo.at(herePos) + getApart(herePos, toPosition))
            {
                continue;
            }

            for (const auto nextPos : graph.getNeighbors(herePos))
            {
                const auto step = costTo.at(herePos) + 1;
                const auto foundCost = costTo.find(nextPos);

                if (foundCost != costTo.end()
                    && foundCost->second <= step)
                {
                    continue;
                }

                costTo[nextPos] = step;
                cameFromPos[nextPos] = herePos;
                open.push(Entry{step + getApart(nextPos, toPosition), nextPos});
            }
        }

        return std::nullopt;
    } // GCOVR_EXCL_LINE

}
