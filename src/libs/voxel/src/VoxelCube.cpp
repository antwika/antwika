#include "antwika/voxel/VoxelCube.hpp"

#include <utility>

#include "antwika/voxel/KindTraits.hpp"
#include "antwika/voxel/VoxelDetail.hpp"
#include "antwika/voxel/VoxelStairs.hpp"

namespace antwika::voxel
{

    std::int32_t cubeTop(const std::int32_t cube)
    {
        return (cube * kCubeSide) + kCubeSide - 1;
    }

    std::int32_t cubeIndexOfLevel(const std::int32_t level)
    {
        const auto underLevel = level < 0 ? level - kCubeSide + 1 : level;

        return underLevel / kCubeSide;
    }

    namespace
    {
        [[nodiscard]] std::int32_t lowestOf(const std::int32_t place)
        {
            const auto cubeOffset = place % kCubeSide;

            return place - (
                cubeOffset < 0 ? cubeOffset + kCubeSide : cubeOffset);
        }

        constexpr std::array<VoxelPosition, 4> kAboutACubePositions{
            VoxelPosition{.x = 1},
            VoxelPosition{.x = -1},
            VoxelPosition{.z = 1},
            VoxelPosition{.z = -1}};

        [[nodiscard]] bool groundBeside(
            const Voxels &standingVoxels,
            const VoxelPosition cornerPosition,
            const VoxelPosition stepPosition,
            const std::int32_t reach)
        {
            for (
            std::int32_t alongIndex = 0; alongIndex < kCubeSide; ++alongIndex)
            {
                const auto besidePosition = VoxelPosition{
                    .x = cornerPosition.x
                         + (stepPosition.x * kCubeSide)
                         + (stepPosition.x == 0 ? alongIndex : 0),
                    .y = reach,
                    .z = cornerPosition.z
                         + (stepPosition.z * kCubeSide)
                         + (stepPosition.z == 0 ? alongIndex : 0)};

                const auto foundVoxel = standingVoxels.find(besidePosition);

                if (foundVoxel != standingVoxels.end()
                    && !isRamped(foundVoxel->second.kind))
                {
                    return true;
                }
            }

            return false;
        }

        [[nodiscard]] bool standsBeside(
            const Voxels &standingVoxels,
            const VoxelPosition cornerPosition,
            const VoxelPosition stepPosition)
        {
            for (std::int32_t upIndex = 0; upIndex < kCubeSide; ++upIndex)
            {
                for (std::int32_t alongIndex = 0; alongIndex < kCubeSide;
                     ++alongIndex)
                {
                    const auto besidePosition = VoxelPosition{
                        .x = cornerPosition.x
                             + (stepPosition.x * kCubeSide)
                             + (stepPosition.x == 0 ? alongIndex : 0),
                        .y = cornerPosition.y + upIndex,
                        .z = cornerPosition.z
                             + (stepPosition.z * kCubeSide)
                             + (stepPosition.z == 0 ? alongIndex : 0)};

                    if (standingVoxels.contains(besidePosition))
                    {
                        return true;
                    }
                }
            }

            return false;
        }

        [[nodiscard]] bool isAutoFacedRamp(
            const Voxels &voxels, const VoxelPosition position)
        {
            auto foundAny = false;

            for (const auto place : cubeCells(cubeCornerOf(position)))
            {
                const auto foundVoxel = voxels.find(place);

                if (foundVoxel == voxels.end())
                {
                    continue;
                }

                if (!isRamped(foundVoxel->second.kind)
                    || foundVoxel->second.facing != Facing::Any)
                {
                    return false;
                }

                foundAny = true;
            }

            return foundAny;
        }

        [[nodiscard]] bool standsAs(
            const Voxels &voxels,
            const VoxelPosition position,
            const Voxels &wantedVoxels)
        {
            std::size_t standing = 0;

            for (const auto place : cubeCells(cubeCornerOf(position)))
            {
                if (!voxels.contains(place))
                {
                    continue;
                }

                if (!wantedVoxels.contains(place))
                {
                    return false;
                }

                ++standing;
            }

            return standing == wantedVoxels.size();
        }
    }

    Side facing(const Side side)
    {
        struct SideRow final
        {
            Side side;
            Side facingSide;
        };

        constexpr std::array<SideRow, kFaceSides> kSideRows{{
            {Side::Top, Side::Bottom},
            {Side::Bottom, Side::Top},
            {Side::Left, Side::Right},
            {Side::Right, Side::Left}}};

        static_assert(enums::tagsInOrder(kSideRows, &SideRow::side));

        return enums::lookup(kSideRows, side).facingSide;
    }

    FaceEdge facing(const FaceEdge edge)
    {
        return FaceEdge{
            .side = facing(edge.side), .edge = edge.edge};
    }

    VoxelPosition cubeCornerOf(const VoxelPosition position)
    {
        return VoxelPosition{
            .x = lowestOf(position.x),
            .y = lowestOf(position.y),
            .z = lowestOf(position.z)};
    }

    std::vector<VoxelPosition> cubeCells(const VoxelPosition cornerPosition)
    {
        std::vector<VoxelPosition> positions;

        positions.reserve(kCubeVoxels);

        for (std::int32_t z = 0; z < kCubeSide; ++z)
        {
            for (std::int32_t y = 0; y < kCubeSide; ++y)
            {
                for (std::int32_t x = 0; x < kCubeSide; ++x)
                {
                    positions.push_back(
                        VoxelPosition{
                            .x = cornerPosition.x + x,
                            .y = cornerPosition.y + y,
                            .z = cornerPosition.z + z});
                }
            }
        }

        return positions;
    } // GCOVR_EXCL_LINE

    Voxels expandCubesToVoxels(const Voxels &cubeVoxels)
    {
        Voxels expandedVoxels;

        for (const auto &[position, material] : cubeVoxels)
        {
            for (const auto place : cubeCells(
                     VoxelPosition{
                         .x = position.x * kCubeSide,
                         .y = position.y * kCubeSide,
                         .z = position.z * kCubeSide}))
            {
                expandedVoxels[place] = material;
            }
        }

        return expandedVoxels;
    } // GCOVR_EXCL_LINE

    VoxelPosition rampDirectionFor(
        const Voxels &filledVoxels, const VoxelPosition position)
    {
        const auto corner = cubeCornerOf(position);
        const auto top = corner.y + kCubeSide - 1;

        for (const auto wantsAWayIn : {true, false})
        {
            for (const auto reach : {top, corner.y})
            {
                for (const auto step : kAboutACubePositions)
                {
                    if (!groundBeside(filledVoxels, corner, step, reach))
                    {
                        continue;
                    }

                    if (wantsAWayIn
                        && standsBeside(
                            filledVoxels, corner, detail::opposite(step)))
                    {
                        continue;
                    }

                    return step;
                }
            }
        }

        return kAboutACubePositions.front();
    }

    Voxels cubeVoxels(
        const VoxelPosition cornerPosition,
        const Kind kind,
        const VoxelPosition climbPosition)
    {
        Voxels grownVoxels;

        if (!isRamped(kind))
        {
            for (const auto place : cubeCells(cornerPosition))
            {
                grownVoxels[place] = VoxelMaterial{.kind = kind};
            }

            return grownVoxels;
        }

        const auto alongX = climbPosition.x != 0;
        const auto forward =
            alongX ? climbPosition.x > 0 : climbPosition.z > 0;
        const auto lowStep = forward ? 0 : kCubeSide - 1;
        const auto highStep = forward ? kCubeSide - 1 : 0;

        for (std::int32_t acrossIndex = 0; acrossIndex < kCubeSide;
             ++acrossIndex)
        {
            for (const auto &[step, upStep] :
                 {std::pair{lowStep, 0},
                  std::pair{highStep, 0},
                  std::pair{highStep, kCubeSide - 1}})
            {
                grownVoxels[VoxelPosition{
                    .x = cornerPosition.x + (alongX ? step : acrossIndex),
                    .y = cornerPosition.y + upStep,
                    .z = cornerPosition.z + (alongX ? acrossIndex : step)}] =
                    VoxelMaterial{.kind = Kind::Ramp};
            }
        }

        return grownVoxels;
    } // GCOVR_EXCL_LINE

    Voxels withBlockAt(
        const Voxels &filledVoxels,
        const VoxelPosition position,
        const Kind kind,
        const Facing facingOverride)
    {
        auto updatedVoxels = withoutBlockAt(filledVoxels, position);
        const auto climb = facingOverride == Facing::Any
                         ? rampDirectionFor(filledVoxels, position)
                         : stepVectorFor(facingOverride);

        for (const auto &[place, material] :
             cubeVoxels(cubeCornerOf(position), kind, climb))
        {
            updatedVoxels[place] = VoxelMaterial{
                .kind = material.kind,
                .facing = isRamped(kind) ? facingOverride
                                             : Facing::Any};
        }

        return updatedVoxels;
    } // GCOVR_EXCL_LINE

    Voxels withRampsRebuilt(
        const Voxels &filledVoxels, const VoxelPosition position)
    {
        const auto corner = cubeCornerOf(position);
        auto updatedVoxels = filledVoxels;

        for (const auto step : kAboutACubePositions)
        {
            const auto besidePosition = VoxelPosition{
                .x = corner.x + (step.x * kCubeSide),
                .y = corner.y,
                .z = corner.z + (step.z * kCubeSide)};

            if (!isAutoFacedRamp(updatedVoxels, besidePosition))
            {
                continue;
            }

            const auto wantedVoxels = cubeVoxels(
                cubeCornerOf(besidePosition),
                Kind::Ramp,
                rampDirectionFor(updatedVoxels, besidePosition));

            if (standsAs(updatedVoxels, besidePosition, wantedVoxels))
            {
                continue;
            }

            updatedVoxels = withBlockAt(
                updatedVoxels, besidePosition, Kind::Ramp, Facing::Any);
        }

        return updatedVoxels;
    } // GCOVR_EXCL_LINE

    Voxels withoutBlockAt(
        const Voxels &filledVoxels, const VoxelPosition position)
    {
        auto keptVoxels = filledVoxels;

        for (const auto place : cubeCells(cubeCornerOf(position)))
        {
            keptVoxels.erase(place);
        }

        return keptVoxels;
    } // GCOVR_EXCL_LINE

}
