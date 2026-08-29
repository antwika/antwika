#include "antwika/voxel/VoxelOcclusion.hpp"

#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

#include "antwika/voxel/KindTraits.hpp"
#include "antwika/voxel/VoxelCube.hpp"

namespace antwika::voxel
{

    namespace
    {
        constexpr std::array<VoxelPosition, 6> kLiftedColumnPositions{
            VoxelPosition{.y = kCubeSide},
            VoxelPosition{.y = kCubeSide, .z = kCubeSide},
            VoxelPosition{.x = -kCubeSide, .y = kCubeSide, .z = kCubeSide},
            VoxelPosition{.x = kCubeSide, .y = kCubeSide, .z = kCubeSide},
            VoxelPosition{.y = kCubeSide, .z = 2 * kCubeSide},
            VoxelPosition{.y = 2 * kCubeSide, .z = 3 * kCubeSide}};

        constexpr std::array<VoxelPosition, 2> kAcrossColumnPositions{
            VoxelPosition{.x = -kCubeSide, .y = kCubeSide},
            VoxelPosition{.x = kCubeSide, .y = kCubeSide}};

        constexpr std::array<VoxelPosition, 5> kSpreadWayPositions{
            VoxelPosition{.x = 1},
            VoxelPosition{.x = -1},
            VoxelPosition{.y = 1},
            VoxelPosition{.z = 1},
            VoxelPosition{.z = -1}};

        [[nodiscard]] bool isWithinMaskWindow(
            const VoxelPosition columnPosition, const VoxelPosition position)
        {
            const auto arm =
                static_cast<std::int32_t>(kOcclusionMaskWidth) / 2;
            const auto acrossOffset = position.x - columnPosition.x;
            const auto alongOffset = position.z - columnPosition.z;

            return acrossOffset >= -arm && acrossOffset < arm
                   && alongOffset >= -arm && alongOffset < arm
                   && position.y >= 0
                   && position.y
                          < static_cast<std::int32_t>(kOcclusionMaskLevels);
        }

        [[nodiscard]] bool isSolidCube(
            const Voxels &filledVoxels, const VoxelPosition cornerPosition)
        {
            for (const auto cellPosition : getCubeCells(cornerPosition))
            {
                const auto foundVoxel = filledVoxels.find(cellPosition);

                if (foundVoxel != filledVoxels.end()
                    && isSolid(foundVoxel->second.kind))
                {
                    return true;
                }
            }

            return false;
        }

    }

    glm::vec3 getLineOfSight(const glm::vec3 standing)
    {
        return standing + glm::vec3{0.0F, kLineOfSightRise, 0.0F};
    }

    glm::vec3 getUpperLineOfSight(const glm::vec3 standing)
    {
        return standing + glm::vec3{0.0F, kUpperSightRise, 0.0F};
    }

    VoxelPosition getVoxelUnder(const glm::vec3 point)
    {
        return VoxelPosition{
            .x = static_cast<std::int32_t>(std::floor(point.x / kVoxelSide)),
            .y = static_cast<std::int32_t>(std::floor(point.y / kVoxelSide)),
            .z = static_cast<std::int32_t>(std::floor(point.z / kVoxelSide))};
    }

    bool isCubeAbove(
        const Voxels &filledVoxels,
        const glm::vec3 standing,
        const float clearance)
    {
        const auto sightPoint = getUpperLineOfSight(standing);
        const auto lowCorner =
            cubeCornerOf(getVoxelUnder(sightPoint - glm::vec3{clearance}));
        const auto highCorner =
            cubeCornerOf(getVoxelUnder(sightPoint + glm::vec3{clearance}));

        for (auto x = lowCorner.x; x <= highCorner.x; x += kCubeSide)
        {
            for (auto y = lowCorner.y; y <= highCorner.y; y += kCubeSide)
            {
                for (auto z = lowCorner.z;
                     z <= highCorner.z;
                     z += kCubeSide)
                {
                    if (isSolidCube(
                            filledVoxels,
                            VoxelPosition{.x = x, .y = y, .z = z}))
                    {
                        return true;
                    }
                }
            }
        }

        return false;
    }

    Voxels getOccludingVoxels(
        const Voxels &filledVoxels, const glm::vec3 standing)
    {
        const auto standingPosition = getVoxelUnder(standing);
        const auto cornerPosition = cubeCornerOf(standingPosition);
        const auto lowestLiftedLevel = cornerPosition.y + kCubeSide;

        Voxels voxels;
        std::vector<VoxelPosition> askingPositions;

        const auto lift =
            [&filledVoxels,
             &voxels,
             &askingPositions,
             standingPosition,
             lowestLiftedLevel](const VoxelPosition position)
        {
            if (voxels.size() >= kMaxOccludedVoxels
                || position.y < lowestLiftedLevel
                || !isWithinMaskWindow(standingPosition, position)
                || voxels.contains(position))
            {
                return;
            }

            const auto foundVoxel = filledVoxels.find(position);

            if (foundVoxel == filledVoxels.end()
                || foundVoxel->second.kind == Kind::Water)
            {
                return;
            }

            voxels[position] = foundVoxel->second;
            askingPositions.push_back(position);
        };

        const auto liftColumn =
            [&lift, cornerPosition](const VoxelPosition columnStep)
        {
            for (std::int32_t acrossStep = 0;
                 acrossStep < kCubeSide;
                 ++acrossStep)
            {
                for (std::int32_t alongStep = 0;
                     alongStep < kCubeSide;
                     ++alongStep)
                {
                    for (auto level = cornerPosition.y + columnStep.y;
                         level < static_cast<std::int32_t>(
                             kOcclusionMaskLevels);
                         ++level)
                    {
                        lift(
                            VoxelPosition{
                                .x = cornerPosition.x + columnStep.x
                                     + acrossStep,
                                .y = level,
                                .z = cornerPosition.z + columnStep.z
                                     + alongStep});
                    }
                }
            }
        };

        for (const auto columnStep : kLiftedColumnPositions)
        {
            liftColumn(columnStep);
        }

        for (const auto columnStep : kAcrossColumnPositions)
        {
            const VoxelPosition besideCornerPosition{
                .x = cornerPosition.x + columnStep.x,
                .y = cornerPosition.y,
                .z = cornerPosition.z + columnStep.z};

            if (isSolidCube(filledVoxels, besideCornerPosition))
            {
                continue;
            }

            liftColumn(columnStep);
        }

        while (!askingPositions.empty())
        {
            const auto nextPosition = askingPositions.back();

            askingPositions.pop_back();

            for (const auto way : kSpreadWayPositions)
            {
                lift(
                    VoxelPosition{
                        .x = nextPosition.x + way.x,
                        .y = nextPosition.y + way.y,
                        .z = nextPosition.z + way.z});
            }
        }

        return voxels;
    } // GCOVR_EXCL_LINE

}
