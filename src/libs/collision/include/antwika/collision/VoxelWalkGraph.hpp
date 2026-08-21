#pragma once

#include <set>
#include <vector>

#include <antwika/pathfinding/IWalkGraph.hpp>
#include <antwika/collision/Collision.hpp>

namespace antwika::collision
{

    class VoxelWalkGraph final : public pathfinding::IWalkGraph
    {
    public:
        explicit VoxelWalkGraph(const std::set<voxel::VoxelCell> &filledCells)
            : filledCells(&filledCells)
        {
        }

        [[nodiscard]] std::vector<pathfinding::GridPos> neighbors(
            const pathfinding::GridPos fromPos) const override
        {
            std::vector<pathfinding::GridPos> gridPositions;
            const auto feet =
                (static_cast<float>(fromPos.y) + 0.5F)
                * voxel::kVoxelSide;

            for (const auto &[byX, byZ] :
                 {std::pair{1, 0},
                  std::pair{-1, 0},
                  std::pair{0, 1},
                  std::pair{0, -1}})
            {
                const auto supportingCell = supportingVoxel(
                    *filledCells, fromPos.x + byX, fromPos.z + byZ, feet);

                if (supportingCell.has_value())
                {
                    gridPositions.push_back(
                        pathfinding::GridPos{
                            .x = supportingCell->x,
                            .y = supportingCell->y,
                            .z = supportingCell->z});
                }
            }

            return gridPositions;
        } // GCOVR_EXCL_LINE

    private:
        const std::set<voxel::VoxelCell> *filledCells;
    };

}
