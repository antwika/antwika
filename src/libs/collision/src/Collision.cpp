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
        [[nodiscard]] std::int32_t columnOf(const float coordinate)
        {
            return static_cast<std::int32_t>(
                std::floor(coordinate / voxel::kVoxelSide));
        }

        [[nodiscard]] std::int32_t lastColumnOf(const float coordinate)
        {
            return static_cast<std::int32_t>(
                       std::ceil(coordinate / voxel::kVoxelSide))
                   - 1;
        }

        constexpr float kHeightSlack = 0.001F;

        [[nodiscard]] constexpr bool nearlyAtLeast(
            const float height, const float bound) noexcept
        {
            return height >= bound - kHeightSlack;
        }

        [[nodiscard]] constexpr bool nearlyAtMost(
            const float height, const float bound) noexcept
        {
            return height <= bound + kHeightSlack;
        }

        [[nodiscard]] constexpr bool nearlyWithin(
            const float height,
            const float lowBound,
            const float highBound) noexcept
        {
            return nearlyAtLeast(height, lowBound)
                   && nearlyAtMost(height, highBound);
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

        [[nodiscard]] std::int32_t stoodOn(const float feet)
        {
            return lastColumnOf(feet);
        }

        [[nodiscard]] component::Position movedBy(
            const voxel::Voxels &filledVoxels,
            const component::Position position,
            const float byX,
            const float byZ)
        {
            const auto x = position.x + byX;
            const auto z = position.z + byZ;

            return groundHeightUnderFootprint(filledVoxels, x, z, position.y)
                .transform([&](const float footing) {
                    return component::Position{
                        .x = x,
                        .y = std::max(footing, position.y),
                        .z = z};
                })
                .value_or(position);
        }

        [[nodiscard]] component::Position snappedToGround(
            const voxel::Voxels &filledVoxels,
            const component::Position position)
        {
            return groundHeightUnderFootprint(
                       filledVoxels, position.x, position.z, position.y)
                .transform([&](const float footing) {
                    return component::Position{
                        .x = position.x,
                        .y = std::max(footing, position.y - kFallSpeed),
                        .z = position.z};
                })
                .value_or(position);
        }

        [[nodiscard]] std::optional<std::pair<
            std::int32_t,
            std::int32_t>>
        ladderRungs(
            const voxel::Voxels &filledVoxels,
            const std::int32_t x,
            const std::int32_t z)
        {
            std::optional<std::int32_t> lowLevel;
            std::optional<std::int32_t> highLevel;

            for (const auto &[position, material] : filledVoxels)
            {
                if (position.x != x || position.z != z
                    || !voxel::isClimbable(material.kind))
                {
                    continue;
                }

                lowLevel = std::min(lowLevel.value_or(position.y), position.y);
                highLevel =
                    std::max(highLevel.value_or(position.y), position.y);
            }

            if (!lowLevel.has_value())
            {
                return std::nullopt;
            }

            return std::pair{*lowLevel, *highLevel};
        }

        [[nodiscard]] float distanceBetween(
            const component::Position position, const gfx::Vec3 fromPosition)
        {
            const auto byX = position.x - fromPosition.x;
            const auto byZ = position.z - fromPosition.z;

            return (byX * byX) + (byZ * byZ);
        }
    }

    gfx::Vec3 positionOf(const component::Position position)
    {
        return gfx::Vec3{position.x, position.y, position.z};
    }

    component::Position positionFrom(const gfx::Vec3 position)
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

    std::optional<voxel::VoxelCell> supportingVoxel(
        const voxel::Voxels &filledVoxels,
        const std::int32_t x,
        const std::int32_t z,
        const float feet)
    {
        const auto underCell = stoodOn(feet);

        for (std::int32_t step = 1; step >= -kMaxFallDepth; --step)
        {
            const voxel::VoxelPosition groundPosition{
                .x = x,
                .y = underCell + step,
                .z = z};
            const auto foundVoxel = filledVoxels.find(groundPosition);

            if (foundVoxel == filledVoxels.end()
                || voxel::isClimbable(foundVoxel->second.kind))
            {
                continue;
            }

            return hasHeadroom(filledVoxels, groundPosition)
                       ? std::optional<voxel::VoxelCell>{
                             voxel::VoxelCell{
                        .position = foundVoxel->first,
                        .material = foundVoxel->second}}
                       : std::nullopt;
        }

        return std::nullopt;
    }

    std::optional<float> groundHeightAtColumn(
        const voxel::Voxels &filledVoxels,
        const std::int32_t x,
        const std::int32_t z,
        const float feet)
    {
        const auto groundCell = supportingVoxel(filledVoxels, x, z, feet);

        if (!groundCell.has_value())
        {
            return std::nullopt;
        }

        const auto top = topOf(groundCell->position.y);

        return voxel::isSwimmable(groundCell->material.kind)
                                 ? top - (voxel::kVoxelSide / 2.0F)
                                 : top;
    }

    float groundHeightOn(
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

        const auto climb = voxel::inferredRampDirection(
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

    std::optional<float> groundHeightUnderFootprint(
        const voxel::Voxels &filledVoxels,
        const float x,
        const float z,
        const float feet)
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
                const auto groundCell =
                    supportingVoxel(filledVoxels, columnIndex, rowIndex, feet);

                if (!groundCell.has_value())
                {
                    return std::nullopt;
                }

                const auto footing = groundHeightOn(
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

    std::optional<component::Position> restPositionOverColumn(
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

    std::optional<component::Position> spawnPosition(
        const voxel::Voxels &filledVoxels)
    {
        const auto middle = voxelmap::voxelsCenter(filledVoxels);
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
            const auto span = distanceBetween(stoodPosition, middle);

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

    namespace
    {
        [[nodiscard]] std::optional<component::Position> climbedOnLadder(
            const voxel::Voxels &filledVoxels,
            const component::Position position,
            const component::Velocity velocity)
        {
            const auto rungs = ladderRungs(
                filledVoxels, columnOf(position.x), columnOf(position.z));

            if (!rungs.has_value())
            {
                return std::nullopt;
            }

            const auto foot = topOf(rungs->first) - voxel::kVoxelSide;
            const auto head = topOf(rungs->second);

            if (!nearlyWithin(position.y, foot, head))
            {
                return std::nullopt;
            }

            const auto pace =
                kWalkSpeed * velocity.speedMultiplier * kRampSpeedFactor;
            auto liftedPosition = position;

            liftedPosition.y = std::clamp(
                position.y + (-velocity.velocityZ * pace), foot, head);

            if (velocity.velocityX != 0.0F)
            {
                liftedPosition = movedBy(
                    filledVoxels,
                    liftedPosition,
                    velocity.velocityX * pace,
                    0.0F);
            }

            const auto leavingTop = nearlyAtLeast(liftedPosition.y, head)
                                    && velocity.velocityZ < 0.0F;
            const auto leavingFoot = nearlyAtMost(liftedPosition.y, foot)
                                     && velocity.velocityZ > 0.0F;

            if (leavingTop || leavingFoot)
            {
                liftedPosition = movedBy(
                    filledVoxels,
                    liftedPosition,
                    0.0F,
                    velocity.velocityZ * pace);
            }

            return liftedPosition;
        }
    }

    component::Position movedWithCollision(
        const voxel::Voxels &filledVoxels,
        const component::Position position,
        const component::Velocity velocity)
    {
        const auto climbedPosition =
            climbedOnLadder(filledVoxels, position, velocity);

        if (climbedPosition.has_value())
        {
            return *climbedPosition;
        }

        const auto walkSpeed = std::sqrt(
            (velocity.velocityX * velocity.velocityX)
            + (velocity.velocityZ * velocity.velocityZ));

        const auto stoodPosition = supportingVoxel(
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

        return snappedToGround(
            filledVoxels,
            movedBy(
                filledVoxels,
                movedBy(filledVoxels, position, byX, 0.0F),
                0.0F,
                byZ));
    }

    std::array<voxel::VoxelPosition, 2> stoodCells(
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
