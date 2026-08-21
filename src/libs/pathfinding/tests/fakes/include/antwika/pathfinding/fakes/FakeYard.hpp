#pragma once

#include <cstdint>
#include <set>
#include <utility>
#include <vector>

#include <antwika/pathfinding/IWalkGraph.hpp>

namespace antwika::pathfinding::fakes
{

    class FakeYard final : public IWalkGraph
    {
    public:
        FakeYard(const std::int32_t side, std::set<GridPos> wallPositions)
            : side(side), wallPositions(std::move(wallPositions))
        {
        }

        [[nodiscard]] std::vector<GridPos> neighbors(
            const GridPos fromPos) const override
        {
            std::vector<GridPos> gridPositions;

            for (const auto &[byX, byZ] :
                 {std::pair{1, 0},
                  std::pair{-1, 0},
                  std::pair{0, 1},
                  std::pair{0, -1}})
            {
                const GridPos nextPos{
                    .x = fromPos.x + byX, .z = fromPos.z + byZ};

                if (nextPos.x < 0 || nextPos.z < 0 || nextPos.x >= side
                    || nextPos.z >= side || wallPositions.contains(nextPos))
                {
                    continue;
                }

                gridPositions.push_back(nextPos);
            }

            return gridPositions;
        } // GCOVR_EXCL_LINE

    private:
        std::int32_t side;
        std::set<GridPos> wallPositions;
    };

}
