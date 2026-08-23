#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <set>

#include <antwika/voxel/VoxelCube.hpp>
#include <antwika/voxel/VoxelDetail.hpp>
#include <antwika/voxel/VoxelStairs.hpp>
#include <antwika/voxel/KindTraits.hpp>

namespace antwika::voxel
{
    using namespace detail;

    namespace detail
    {

        VoxelPosition offsetBy(
            const VoxelPosition fromPosition,
            const VoxelPosition byPosition)
        {
            return VoxelPosition{
                .x = fromPosition.x + byPosition.x,
                .y = fromPosition.y + byPosition.y,
                .z = fromPosition.z + byPosition.z};
        }

        VoxelPosition opposite(const VoxelPosition stepPosition)
        {
            return VoxelPosition{
                .x = -stepPosition.x,
                .y = -stepPosition.y,
                .z = -stepPosition.z};
        }

        constexpr std::array<VoxelPosition, 4> kAboutPositions{
            VoxelPosition{.x = 1},
            VoxelPosition{.x = -1},
            VoxelPosition{.z = 1},
            VoxelPosition{.z = -1}};

        constexpr VoxelPosition kBelowPosition{.y = -1};

        std::optional<Kind> kindAt(
            const Voxels &filledVoxels, const VoxelPosition position)
        {
            const auto foundVoxel = filledVoxels.find(position);

            return foundVoxel == filledVoxels.end()
                              ? std::nullopt
                              : std::optional{foundVoxel->second.kind};
        }

        std::optional<VoxelMaterial> materialAt(
            const Voxels &filledVoxels, const VoxelPosition position)
        {
            const auto foundVoxel = filledVoxels.find(position);

            return foundVoxel == filledVoxels.end()
                              ? std::nullopt
                              : std::optional{foundVoxel->second};
        }

        [[nodiscard]] bool groundBeside(
            const Voxels &filledVoxels,
            const VoxelPosition position,
            const VoxelPosition stepPosition)
        {
            const auto corner = cubeCornerOf(position);

            for (std::int32_t upIndex = 0; upIndex < kCubeSide; ++upIndex)
            {
                for (std::int32_t alongIndex = 0; alongIndex < kCubeSide;
                     ++alongIndex)
                {
                    const auto besidePosition = VoxelPosition{
                        .x = corner.x
                             + (stepPosition.x != 0
                                            ? (stepPosition.x > 0
                                                   ? kCubeSide
                                                   : -1)
                                            : alongIndex),
                        .y = corner.y + upIndex,
                        .z = corner.z
                             + (stepPosition.z != 0
                                            ? (stepPosition.z > 0
                                                   ? kCubeSide
                                                   : -1)
                                            : alongIndex)};

                    if (kindAt(filledVoxels, besidePosition) == Kind::Normal)
                    {
                        return true;
                    }
                }
            }

            return false;
        }

        [[nodiscard]] std::optional<VoxelPosition> shapedClimb(
            const Voxels &filledVoxels, const VoxelPosition position)
        {
            const auto corner = cubeCornerOf(position);
            const auto top = corner.y + kCubeSide - 1;

            std::set<std::int32_t> acrossX;
            std::set<std::int32_t> acrossZ;

            for (std::int32_t offX = 0; offX < kCubeSide; ++offX)
            {
                for (std::int32_t offZ = 0; offZ < kCubeSide; ++offZ)
                {
                    const auto probePosition = VoxelPosition{
                        .x = corner.x + offX,
                        .y = top,
                        .z = corner.z + offZ};

                    if (isRamped(kindAt(filledVoxels, probePosition)))
                    {
                        acrossX.insert(offX);
                        acrossZ.insert(offZ);
                    }
                }
            }

            if (acrossX.size() == 1 && acrossZ.size() > 1)
            {
                return VoxelPosition{
                    .x = *acrossX.begin() == 0 ? -1 : 1};
            }

            if (acrossZ.size() == 1 && acrossX.size() > 1)
            {
                return VoxelPosition{
                    .z = *acrossZ.begin() == 0 ? -1 : 1};
            }

            return std::nullopt;
        }

        [[nodiscard]] VoxelPosition climbWithin(
            const Voxels &filledVoxels, const VoxelPosition position)
        {
            if (const auto shapedStep = shapedClimb(filledVoxels, position);
                shapedStep.has_value())
            {
                return *shapedStep;
            }

            const auto material = materialAt(filledVoxels, position);

            if (material.has_value() && material->facing != Facing::Any)
            {
                return stepVectorFor(material->facing);
            }

            for (const auto step : kAboutPositions)
            {
                if (groundBeside(filledVoxels, position, step)
                    && !groundBeside(
                        filledVoxels, position, opposite(step)))
                {
                    return step;
                }
            }

            for (const auto step : kAboutPositions)
            {
                if (groundBeside(filledVoxels, position, step))
                {
                    return step;
                }
            }

            const auto fills = [&filledVoxels](const VoxelPosition place)
            { return effectiveKindAt(filledVoxels, place) == Kind::Normal; };

            for (const auto step : kAboutPositions)
            {
                if (fills(offsetBy(position, step))
                    && !fills(offsetBy(position, opposite(step))))
                {
                    return step;
                }
            }

            const auto abovePosition =
                offsetBy(position, VoxelPosition{.y = 1});

            if (isRamped(kindAt(filledVoxels, abovePosition)))
            {
                return climbWithin(filledVoxels, abovePosition);
            }

            for (const auto step : kAboutPositions)
            {
                const auto open = offsetBy(position, opposite(step));

                if (!filledVoxels.contains(open)
                    && isRamped(
                        kindAt(
                            filledVoxels,
                            offsetBy(open, kBelowPosition))))
                {
                    return step;
                }
            }

            return kAboutPositions.front();
        }

        [[nodiscard]] StairHalf levelWithin(
            const Voxels &filledVoxels, const VoxelPosition position)
        {
            if (!isRamped(kindAt(filledVoxels, position)))
            {
                return StairHalf::Any;
            }

            return isRamped(
                       kindAt(
                           filledVoxels,
                           offsetBy(position, kBelowPosition)))
                       ? StairHalf::Upper
                       : StairHalf::Lower;
        }

        bool isRampStep(
            const Voxels &filledVoxels, const VoxelPosition position)
        {
            return isRamped(kindAt(filledVoxels, position))
                   && !filledVoxels.contains(
                       offsetBy(position, VoxelPosition{.y = 1}));
        }

        std::optional<Kind> effectiveKindAt(
            const Voxels &filledVoxels, const VoxelPosition position)
        {
            const auto voxelKind = kindAt(filledVoxels, position);

            if (!isRamped(voxelKind))
            {
                return voxelKind;
            }

            return isRampStep(filledVoxels, position) ? Kind::Ramp
                                                      : Kind::Normal;
        }

    }

    VoxelPosition inferredRampDirection(
        const Voxels &filledVoxels, const VoxelPosition position)
    {
        return climbWithin(filledVoxels, position);
    }

    Facing facingOfStep(const VoxelPosition climbPosition)
    {
        if (climbPosition.x > 0)
        {
            return Facing::East;
        }

        if (climbPosition.x < 0)
        {
            return Facing::West;
        }

        if (climbPosition.z > 0)
        {
            return Facing::South;
        }

        return climbPosition.z < 0 ? Facing::North : Facing::Any;
    }

    VoxelPosition stepVectorFor(const Facing facing)
    {
        switch (facing)
        {
        case Facing::East:
            return VoxelPosition{.x = 1};
        case Facing::West:
            return VoxelPosition{.x = -1};
        case Facing::North:
            return VoxelPosition{.z = -1};
        case Facing::South:
            return VoxelPosition{.z = 1};
        case Facing::Any:
            break;
        }

        return VoxelPosition{};
    }

    StairHalf stairHalfOf(
        const Voxels &filledVoxels, const VoxelPosition position)
    {
        return levelWithin(filledVoxels, position);
    }

}
