#include "antwika/voxel/VoxelOcclusion.hpp"

#include <glm/geometric.hpp>

#include <cmath>
#include <cstdlib>
#include <cstdint>
#include <optional>
#include <unordered_set>
#include <vector>

#include "antwika/voxel/KindTraits.hpp"
#include "antwika/voxel/VoxelCube.hpp"

namespace antwika::voxel
{

    namespace
    {
        using PositionSet =
            std::unordered_set<VoxelPosition, VoxelPositionHash>;


        [[nodiscard]] std::optional<VoxelPosition> getRoofOver(
            const Voxels &filledVoxels,
            const VoxelPosition columnPosition,
            const std::int32_t lowest)
        {
            for (std::int32_t level = lowest;
                 level < lowest + kRoofSearchLevels;
                 ++level)
            {
                const VoxelPosition overheadPosition{
                    .x = columnPosition.x,
                    .y = level,
                    .z = columnPosition.z};
                const auto foundVoxel = filledVoxels.find(overheadPosition);

                if (foundVoxel != filledVoxels.end()
                    && foundVoxel->second.kind != Kind::Water)
                {
                    return overheadPosition;
                }
            }

            return std::nullopt;
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

        [[nodiscard]] PositionSet getShellAbout(
            const Voxels &filledVoxels,
            const VoxelPosition columnPosition,
            const std::int32_t lowest,
            const std::int32_t roofLevel)
        {
            const auto arm =
                static_cast<std::int32_t>(kOcclusionMaskWidth) / 2;

            const auto withinWindow = [columnPosition, lowest, roofLevel](
                                          const VoxelPosition position)
            {
                return position.y >= lowest && position.y < roofLevel
                       && std::abs(position.x - columnPosition.x) <= arm
                       && std::abs(position.z - columnPosition.z) <= arm;
            };

            const auto standsIn = [&filledVoxels](const VoxelPosition position)
            {
                const auto foundVoxel = filledVoxels.find(position);

                return foundVoxel != filledVoxels.end()
                       && foundVoxel->second.kind != Kind::Water;
            };

            PositionSet shellPositions;
            const VoxelPosition fromPosition{
                .x = columnPosition.x, .y = lowest, .z = columnPosition.z};

            if (!withinWindow(fromPosition) || standsIn(fromPosition))
            {
                return shellPositions;
            }

            PositionSet airPositions{fromPosition};
            std::vector<VoxelPosition> askingPositions{fromPosition};

            while (!askingPositions.empty())
            {
                const auto nextPosition = askingPositions.back();

                askingPositions.pop_back();

                for (const auto way :
                     {VoxelPosition{.x = 1}, VoxelPosition{.x = -1},
                      VoxelPosition{.y = 1}, VoxelPosition{.y = -1},
                      VoxelPosition{.z = 1}, VoxelPosition{.z = -1}})
                {
                    const VoxelPosition besidePosition{
                        .x = nextPosition.x + way.x,
                        .y = nextPosition.y + way.y,
                        .z = nextPosition.z + way.z};

                    if (!withinWindow(besidePosition))
                    {
                        continue;
                    }

                    if (standsIn(besidePosition))
                    {
                        shellPositions.insert(besidePosition);

                        continue;
                    }

                    if (airPositions.insert(besidePosition).second)
                    {
                        askingPositions.push_back(besidePosition);
                    }
                }
            }

            return shellPositions;
        } // GCOVR_EXCL_LINE

        void takeWithin(
            const Voxels &filledVoxels,
            const VoxelPosition columnPosition,
            const std::int32_t lowest,
            const std::int32_t levelCount,
            const PositionSet &shellPositions,
            Voxels &voxels)
        {
            const auto arm =
                static_cast<std::int32_t>(kOcclusionMaskWidth) / 2;

            for (auto x = columnPosition.x - arm; x <= columnPosition.x + arm;
                 ++x)
            {
                for (auto y = lowest; y < lowest + levelCount; ++y)
                {
                    for (auto z = columnPosition.z - arm;
                         z <= columnPosition.z + arm;
                         ++z)
                    {
                        if (voxels.size() >= kMaxOccludedVoxels)
                        {
                            return;
                        }

                        const VoxelPosition standingPosition{
                            .x = x, .y = y, .z = z};
                        const auto foundVoxel =
                            filledVoxels.find(standingPosition);

                        if (foundVoxel == filledVoxels.end()
                            || foundVoxel->second.kind == Kind::Water
                            || shellPositions.contains(standingPosition))
                        {
                            continue;
                        }

                        voxels[standingPosition] = foundVoxel->second;
                    }
                }
            }
        }

        void liftWhatIsNotTheRoom(
            const Voxels &filledVoxels,
            const VoxelPosition columnPosition,
            const std::int32_t lowest,
            const std::int32_t roofLevel,
            Voxels &voxels)
        {
            const auto shellPositions =
                getShellAbout(filledVoxels, columnPosition, lowest, roofLevel);

            if (shellPositions.empty())
            {
                return;
            }

            takeWithin(
                filledVoxels,
                columnPosition,
                lowest,
                roofLevel - lowest,
                shellPositions,
                voxels);
        }

        void liftTheRoof(
            const Voxels &filledVoxels,
            const VoxelPosition columnPosition,
            const std::int32_t roofLevel,
            Voxels &voxels)
        {
            takeWithin(
                filledVoxels,
                columnPosition,
                roofLevel,
                static_cast<std::int32_t>(kOcclusionMaskLevels),
                {},
                voxels);
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
        Voxels voxels;
        const auto columnPosition = getVoxelUnder(getLineOfSight(standing));
        const auto overheadPosition =
            getRoofOver(filledVoxels, columnPosition, columnPosition.y);

        if (!overheadPosition.has_value())
        {
            return voxels;
        }

        liftTheRoof(
            filledVoxels, columnPosition, overheadPosition->y, voxels);
        liftWhatIsNotTheRoom(
            filledVoxels,
            columnPosition,
            columnPosition.y,
            overheadPosition->y,
            voxels);

        return voxels;
    } // GCOVR_EXCL_LINE

}
