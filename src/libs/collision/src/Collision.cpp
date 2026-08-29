#include "antwika/collision/Collision.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <utility>

#include <antwika/component/Position.hpp>
#include <antwika/component/Velocity.hpp>
#include <antwika/voxel/KindTraits.hpp>

namespace antwika::collision
{

    namespace
    {
        [[nodiscard]] std::int32_t lastColumnOf(const float coordinate)
        {
            return static_cast<std::int32_t>(
                       std::ceil(coordinate / voxel::kVoxelSide))
                   - 1;
        }

        [[nodiscard]] float topOf(const std::int32_t y)
        {
            return (static_cast<float>(y) + 1.0F) * voxel::kVoxelSide;
        }

        [[nodiscard]] std::int32_t levelAt(const float height)
        {
            return static_cast<std::int32_t>(
                std::floor(height / voxel::kVoxelSide));
        }

        [[nodiscard]] std::int32_t getReachLevel(const float feet)
        {
            return levelAt(feet + kWalkerStep);
        }

        [[nodiscard]] float getStepOnto(
            const voxel::VoxelCell groundCell, const float stepUp)
        {
            return voxel::isRamped(groundCell.material.kind)
                       ? kWalkerStep
                       : stepUp;
        }

        [[nodiscard]] float getLowestFootingOn(
            const voxel::VoxelCell groundCell)
        {
            const auto top = topOf(groundCell.position.y);

            if (voxel::isSwimmable(groundCell.material.kind))
            {
                return top - (voxel::kVoxelSide / 2.0F);
            }

            return voxel::isRamped(groundCell.material.kind)
                       ? top - voxel::kVoxelSide
                       : top;
        }

        [[nodiscard]] component::Position getMovedBy(
            const voxel::Voxels &filledVoxels,
            const component::Position position,
            const float byX,
            const float byZ,
            const float stepUp)
        {
            const auto x = position.x + byX;
            const auto z = position.z + byZ;

            return getGroundHeightUnderFootprint( // GCOVR_EXCL_LINE
                       filledVoxels, x, z, position.y, stepUp)
                .transform([&](const float footing) {
                    return component::Position{
                        .x = x,
                        .y = std::max(footing, position.y),
                        .z = z};
                })
                .value_or(position);
        }

        [[nodiscard]] component::Position getSnappedToGround(
            const voxel::Voxels &filledVoxels,
            const component::Position position,
            const float stepUp)
        {
            return getGroundHeightUnderFootprint( // GCOVR_EXCL_LINE
                       filledVoxels, position.x, position.z, position.y, stepUp)
                .transform([&](const float footing) {
                    return component::Position{
                        .x = position.x,
                        .y = std::max(footing, position.y - kFallSpeed),
                        .z = position.z};
                })
                .value_or(position);
        }

        [[nodiscard]] float getDistanceBetween(
            const component::Position position, const geometry::Vec3 fromPosition)
        {
            const auto byX = position.x - fromPosition.x;
            const auto byZ = position.z - fromPosition.z;

            return (byX * byX) + (byZ * byZ);
        }
    }

    std::int32_t columnOf(const float coordinate)
    {
        return static_cast<std::int32_t>(
            std::floor(coordinate / voxel::kVoxelSide));
    }

    geometry::Vec3 positionOf(const component::Position position)
    {
        return geometry::Vec3{position.x, position.y, position.z};
    }

    component::Position positionFrom(const geometry::Vec3 position)
    {
        return component::Position{
            .x = position.x, .y = position.y, .z = position.z};
    }

    bool isSolid(
        const voxel::Voxels &filledVoxels,
        const voxel::VoxelPosition position)
    {
        const auto foundVoxel = filledVoxels.find(position);

        return foundVoxel != filledVoxels.end()
               && voxel::isSolid(foundVoxel->second.kind);
    }

    bool hasHeadroom(
        const voxel::Voxels &filledVoxels,
        const voxel::VoxelPosition groundPosition)
    {
        for (std::int32_t upIndex = 1; upIndex <= kWalkerHeight; ++upIndex)
        {
            const voxel::VoxelPosition overPosition{
                .x = groundPosition.x,
                .y = groundPosition.y + upIndex,
                .z = groundPosition.z};

            if (isSolid(filledVoxels, overPosition))
            {
                return false;
            }
        }

        return true;
    }

    std::optional<voxel::VoxelCell> getSupportingVoxel(
        const voxel::Voxels &filledVoxels,
        const std::int32_t x,
        const std::int32_t z,
        const float feet)
    {
        return getSupportingVoxel(filledVoxels, x, z, feet, kWalkerStep);
    }

    std::optional<voxel::VoxelCell> getSupportingVoxel(
        const voxel::Voxels &filledVoxels,
        const std::int32_t x,
        const std::int32_t z,
        const float feet,
        const float stepUp)
    {
        const auto reachLevel = getReachLevel(feet);

        for (std::int32_t step = 0; step >= -kMaxFallDepth; --step)
        {
            const voxel::VoxelPosition groundPosition{
                .x = x,
                .y = reachLevel + step,
                .z = z};
            const auto foundVoxel = filledVoxels.find(groundPosition);

            if (foundVoxel == filledVoxels.end())
            {
                continue;
            }

            const voxel::VoxelCell groundCell{
                .position = foundVoxel->first,
                .material = foundVoxel->second};

            if (getLowestFootingOn(groundCell)
                > feet + getStepOnto(groundCell, stepUp))
            {
                continue;
            }

            return hasHeadroom(filledVoxels, groundPosition)
                       ? std::optional<voxel::VoxelCell>{groundCell}
                       : std::nullopt;
        }

        return std::nullopt;
    }

    std::optional<float> getGroundHeightAtColumn(
        const voxel::Voxels &filledVoxels,
        const std::int32_t x,
        const std::int32_t z,
        const float feet)
    {
        const auto groundCell = getSupportingVoxel(filledVoxels, x, z, feet);

        if (!groundCell.has_value())
        {
            return std::nullopt;
        }

        const auto top = topOf(groundCell->position.y);

        return voxel::isSwimmable(groundCell->material.kind)
                                 ? top - (voxel::kVoxelSide / 2.0F)
                                 : top;
    }

    float getGroundHeightOn(
        const voxel::Voxels &filledVoxels,
        const voxel::VoxelCell groundCell,
        const float x,
        const float z)
    {
        const auto top = topOf(groundCell.position.y);

        if (voxel::isSwimmable(groundCell.material.kind))
        {
            return top - (voxel::kVoxelSide / 2.0F);
        }

        if (!voxel::isRamped(groundCell.material.kind))
        {
            return top;
        }

        const auto climb = voxel::getInferredRampDirection(
            filledVoxels,
            groundCell.position);
        const auto rising = climb.x != 0;
        const auto way = static_cast<float>(
            rising ? climb.x : climb.z);
        const auto alongOffset =
            rising ? x
                         - ((static_cast<float>(groundCell.position.x) + 0.5F)
                            * voxel::kVoxelSide)
                   : z
                         - ((static_cast<float>(groundCell.position.z) + 0.5F)
                            * voxel::kVoxelSide);
        const auto part = std::clamp(
            ((alongOffset * way) / voxel::kVoxelSide) + 0.5F, 0.0F, 1.0F);

        return top - voxel::kVoxelSide + (part * voxel::kVoxelSide);
    }

    std::optional<float> getGroundHeightUnderFootprint(
        const voxel::Voxels &filledVoxels,
        const float x,
        const float z,
        const float feet)
    {
        return getGroundHeightUnderFootprint(
            filledVoxels, x, z, feet, kWalkerStep);
    }

    std::optional<float> getGroundHeightUnderFootprint(
        const voxel::Voxels &filledVoxels,
        const float x,
        const float z,
        const float feet,
        const float stepUp)
    {
        const auto acrossArm = kFootprintWidth / 2.0F;
        const auto alongArm = kFootprintDepth / 2.0F;
        std::optional<float> highest;

        for (auto columnIndex = columnOf(x - acrossArm);
             columnIndex <= lastColumnOf(x + acrossArm);
             ++columnIndex)
        {
            for (auto rowIndex = columnOf(z - alongArm);
                 rowIndex <= lastColumnOf(z + alongArm);
                 ++rowIndex)
            {
                const auto groundCell = getSupportingVoxel(
                    filledVoxels, columnIndex, rowIndex, feet, stepUp);

                if (!groundCell.has_value())
                {
                    return std::nullopt;
                }

                const auto footing = getGroundHeightOn(
                    filledVoxels,
                    *groundCell,
                    x,
                    z);

                highest =
                    std::max(highest.value_or(footing), footing);
            }
        }

        return highest;
    }

    std::optional<component::Position> getRestPositionOverColumn(
        const voxel::Voxels &filledVoxels,
        const std::int32_t x,
        const std::int32_t z)
    {
        std::optional<std::int32_t> best;

        for (const auto &[position, material] : filledVoxels)
        {
            const auto worse = best.has_value() && position.y <= *best;

            if (position.x != x || position.z != z || worse
                || voxel::isSwimmable(material.kind)
                || !hasHeadroom(filledVoxels, position))
            {
                continue;
            }

            best = position.y;
        }

        if (!best.has_value())
        {
            return std::nullopt;
        }

        return component::Position{
            .x = static_cast<float>(x) * voxel::kVoxelSide,
            .y = topOf(*best),
            .z = static_cast<float>(z) * voxel::kVoxelSide};
    }

    std::optional<component::Position> getSpawnPosition(
        const voxel::Voxels &filledVoxels)
    {
        const auto middle = voxelmap::getVoxelsCenter(filledVoxels);
        std::optional<component::Position> bestPosition;
        auto nearest = 0.0F;

        for (const auto &[position, material] : filledVoxels)
        {
            if (voxel::isSwimmable(material.kind)
                || !hasHeadroom(filledVoxels, position))
            {
                continue;
            }

            const component::Position stoodPosition{
                .x = static_cast<float>(position.x) * voxel::kVoxelSide,
                .y = topOf(position.y),
                .z = static_cast<float>(position.z) * voxel::kVoxelSide};
            const auto span = getDistanceBetween(stoodPosition, middle);

            if (bestPosition.has_value()
                && (span > nearest
                    || (span == nearest && stoodPosition.y <= bestPosition->y)))
            {
                continue;
            }

            bestPosition = stoodPosition;
            nearest = span;
        }

        return bestPosition;
    }

    component::Position getMovedWithCollision(
        const voxel::Voxels &filledVoxels,
        const component::Position position,
        const component::Velocity velocity)
    {
        const auto walkSpeed = std::sqrt(
            (velocity.velocityX * velocity.velocityX)
            + (velocity.velocityZ * velocity.velocityZ));

        const auto stoodPosition = getSupportingVoxel(
            filledVoxels,
            columnOf(position.x),
            columnOf(position.z),
            position.y);
        const auto climbing =
            stoodPosition.has_value()
            && voxel::isRamped(stoodPosition->material.kind);
        const auto wading =
            stoodPosition.has_value()
            && voxel::isSwimmable(stoodPosition->material.kind);
        const auto pace = kWalkSpeed * velocity.speedMultiplier
                          * (climbing ? kRampSpeedFactor : 1.0F)
                          * (wading ? kWaterSpeedFactor : 1.0F)
                          / std::max(walkSpeed, 1.0F);
        const auto byX = velocity.velocityX * pace;
        const auto byZ = velocity.velocityZ * pace;

        const auto stepUp = climbing ? kRampSideStep : kWalkerStep;

        return getSnappedToGround(
            filledVoxels,
            getMovedBy(
                filledVoxels,
                getMovedBy(filledVoxels, position, byX, 0.0F, stepUp),
                0.0F,
                byZ,
                stepUp),
            stepUp);
    }

    std::array<voxel::VoxelPosition, 2> getStoodCells(
        const component::Position position)
    {
        const voxel::VoxelPosition standsInPosition{
            .x = levelAt(position.x),
            .y = levelAt(position.y),
            .z = levelAt(position.z)};

        return {
            standsInPosition,
            voxel::VoxelPosition{
                .x = standsInPosition.x,
                .y = standsInPosition.y - 1,
                .z = standsInPosition.z}};
    } // GCOVR_EXCL_LINE

}
