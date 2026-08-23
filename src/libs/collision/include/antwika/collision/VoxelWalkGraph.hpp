#pragma once

#include <vector>

#include <antwika/pathfinding/IWalkGraph.hpp>
#include <antwika/collision/Collision.hpp>

namespace antwika::collision
{

    class VoxelWalkGraph final : public pathfinding::IWalkGraph
    {
    public:
        explicit VoxelWalkGraph(const voxel::Voxels &filledVoxels)
            : filledVoxels(&filledVoxels)
        {
        }

        [[nodiscard]] std::vector<pathfinding::GridPos> getNeighbors(
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
                const auto supportingCell = getSupportingVoxel(
                    *filledVoxels, fromPos.x + byX, fromPos.z + byZ, feet);

                if (supportingCell.has_value())
                {
                    gridPositions.push_back(
                        pathfinding::GridPos{
                            .x = supportingCell->position.x,
                            .y = supportingCell->position.y,
                            .z = supportingCell->position.z});
                }
            }

            return gridPositions;
        } // GCOVR_EXCL_LINE

    private:
        const voxel::Voxels *filledVoxels;
    };

}
