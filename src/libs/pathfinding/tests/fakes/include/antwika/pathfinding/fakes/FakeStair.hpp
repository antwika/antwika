#pragma once

#include <vector>

#include <antwika/pathfinding/IWalkGraph.hpp>

namespace antwika::pathfinding::fakes
{

    class FakeStair final : public IWalkGraph
    {
    public:
        FakeStair() = default;

        [[nodiscard]] std::vector<GridPos> getNeighbors(
            const GridPos fromPos) const override
        {
            std::vector<GridPos> gridPositions;

            if (fromPos.x < 4)
            {
                gridPositions.push_back(
                    GridPos{.x = fromPos.x + 1, .y = fromPos.y + 1});
            }

            if (fromPos.x > 0)
            {
                gridPositions.push_back(
                    GridPos{.x = fromPos.x - 1, .y = fromPos.y - 1});
            }

            return gridPositions;
        } // GCOVR_EXCL_LINE
    };

}
