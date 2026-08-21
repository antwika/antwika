#include "antwika/collision/Collision.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <utility>

#include <antwika/component/Position.hpp>
#include <antwika/component/Velocity.hpp>

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
            const std::set<voxel::VoxelCell> &filledCells,
            const component::Position position,
            const float byX,
            const float byZ)
        {
            const auto x = position.x + byX;
            const auto z = position.z + byZ;
            const auto footing =
                groundHeightUnderFootprint(filledCells, x, z, position.y);

            if (!footing.has_value())
            {
                return position;
            }

            const auto height = std::max(*footing, position.y);

            return component::Position{.x = x, .y = height, .z = z};
        }

        [[nodiscard]] component::Position snappedToGround(
            const std::set<voxel::VoxelCell> &filledCells,
            const component::Position position)
        {
            const auto footing = groundHeightUnderFootprint(
                filledCells, position.x, position.z, position.y);

            if (!footing.has_value())
            {
                return position;
            }

            const auto fell = std::max(
                *footing, position.y - kFallSpeed);

            return component::Position{
                .x = position.x, .y = fell, .z = position.z};
        }

        [[nodiscard]] std::optional<std::pair<
            std::int32_t,
            std::int32_t>>
        ladderRungs(
            const std::set<voxel::VoxelCell> &filledCells,
            const std::int32_t x,
            const std::int32_t z)
        {
            std::optional<std::int32_t> lowLevel;
            std::optional<std::int32_t> highLevel;

            for (const auto &cell : filledCells)
            {
                if (cell.x != x || cell.z != z
                    || cell.kind != voxel::Kind::Ladder)
                {
                    continue;
                }

                lowLevel = std::min(lowLevel.value_or(cell.y), cell.y);
                highLevel = std::max(highLevel.value_or(cell.y), cell.y);
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
        const std::set<voxel::VoxelCell> &filledCells,
        const voxel::VoxelCell cells)
    {
        const auto foundCell = filledCells.find(cells);

        return foundCell != filledCells.end()
               && foundCell->kind != voxel::Kind::Water
               && foundCell->kind != voxel::Kind::Ladder;
    }

    bool hasHeadroom(
        const std::set<voxel::VoxelCell> &filledCells,
        const voxel::VoxelCell groundCell)
    {
        for (std::int32_t upIndex = 1; upIndex <= kWalkerHeight; ++upIndex)
        {
            const voxel::VoxelCell overCell{
                .x = groundCell.x, .y = groundCell.y + upIndex,
                .z = groundCell.z};

            if (isSolid(filledCells, overCell))
            {
                return false;
            }
        }

        return true;
    }

    std::optional<voxel::VoxelCell> supportingVoxel(
        const std::set<voxel::VoxelCell> &filledCells,
        const std::int32_t x,
        const std::int32_t z,
        const float feet)
    {
        const auto underCell = stoodOn(feet);

        for (std::int32_t step = 1; step >= -kMaxFallDepth; --step)
        {
            const voxel::VoxelCell groundCell{
                .x = x,
                .y = underCell + step,
                .z = z};
            const auto foundCell = filledCells.find(groundCell);

            if (foundCell == filledCells.end()
                || foundCell->kind == voxel::Kind::Ladder)
            {
                continue;
            }

            return hasHeadroom(filledCells, groundCell)
                       ? std::optional<voxel::VoxelCell>{*foundCell}
                       : std::nullopt;
        }

        return std::nullopt;
    }

    std::optional<float> groundHeightAtColumn(
        const std::set<voxel::VoxelCell> &filledCells,
        const std::int32_t x,
        const std::int32_t z,
        const float feet)
    {
        const auto groundCell = supportingVoxel(filledCells, x, z, feet);

        if (!groundCell.has_value())
        {
            return std::nullopt;
        }

        const auto top = topOf(groundCell->y);

        return groundCell->kind == voxel::Kind::Water
                                 ? top - (voxel::kVoxelSide / 2.0F)
                                 : top;
    }

    float groundHeightOn(
        const std::set<voxel::VoxelCell> &filledCells,
        const voxel::VoxelCell groundCell,
        const float x,
        const float z)
    {
        const auto top = topOf(groundCell.y);

        if (groundCell.kind == voxel::Kind::Water)
        {
            return top - (voxel::kVoxelSide / 2.0F);
        }

        if (groundCell.kind != voxel::Kind::Ramp)
        {
            return top;
        }

        const auto climb = voxel::inferredRampDirection(
            filledCells,
            groundCell);
        const auto rising = climb.x != 0;
        const auto way = static_cast<float>(
            rising ? climb.x : climb.z);
        const auto alongOffset =
            rising ? x
                         - ((static_cast<float>(groundCell.x) + 0.5F)
                            * voxel::kVoxelSide)
                   : z
                         - ((static_cast<float>(groundCell.z) + 0.5F)
                            * voxel::kVoxelSide);
        const auto part = std::clamp(
            ((alongOffset * way) / voxel::kVoxelSide) + 0.5F, 0.0F, 1.0F);

        return top - voxel::kVoxelSide + (part * voxel::kVoxelSide);
    }

    std::optional<float> groundHeightUnderFootprint(
        const std::set<voxel::VoxelCell> &filledCells,
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
                    supportingVoxel(filledCells, columnIndex, rowIndex, feet);

                if (!groundCell.has_value())
                {
                    return std::nullopt;
                }

                const auto footing = groundHeightOn(
                    filledCells,
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
        const std::set<voxel::VoxelCell> &filledCells,
        const std::int32_t x,
        const std::int32_t z)
    {
        std::optional<std::int32_t> best;

        for (const auto cell : filledCells)
        {
            const auto worse = best.has_value() && cell.y <= *best;

            if (cell.x != x || cell.z != z || worse
                || cell.kind == voxel::Kind::Water
                || !hasHeadroom(filledCells, cell))
            {
                continue;
            }

            best = cell.y;
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
        const std::vector<voxel::VoxelCell> &cells)
    {
        const std::set<voxel::VoxelCell> filledCells(
            cells.begin(),
            cells.end());
        const auto middle = voxelmap::voxelsCenter(cells);
        std::optional<component::Position> bestPosition;
        auto nearest = 0.0F;

        for (const auto cell : filledCells)
        {
            if (cell.kind == voxel::Kind::Water
                || !hasHeadroom(filledCells, cell))
            {
                continue;
            }

            const component::Position stoodPosition{
                .x = static_cast<float>(cell.x) * voxel::kVoxelSide,
                .y = topOf(cell.y),
                .z = static_cast<float>(cell.z) * voxel::kVoxelSide};
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

    component::Position movedWithCollision(
        const std::set<voxel::VoxelCell> &filledCells,
        const component::Position position,
        const component::Velocity velocity)
    {
        const auto rungs = ladderRungs(
            filledCells,
            columnOf(position.x),
            columnOf(position.z));

        if (rungs.has_value())
        {
            const auto foot =
                topOf(rungs->first) - voxel::kVoxelSide;
            const auto head = topOf(rungs->second);

            if (position.y >= foot - 0.001F
                && position.y <= head + 0.001F)
            {
                const auto pace = kWalkSpeed * velocity.speedMultiplier
                                  * kRampSpeedFactor;
                auto liftedPosition = position;

                liftedPosition.y = std::clamp(
                    position.y + (-velocity.velocityZ * pace),
                    foot,
                    head);

                if (velocity.velocityX != 0.0F)
                {
                    liftedPosition = movedBy(
                        filledCells,
                        liftedPosition,
                        velocity.velocityX * pace,
                        0.0F);
                }

                if ((liftedPosition.y >= head - 0.001F
                     && velocity.velocityZ < 0.0F)
                    || (liftedPosition.y <= foot + 0.001F
                        && velocity.velocityZ > 0.0F))
                {
                    liftedPosition = movedBy(
                        filledCells,
                        liftedPosition,
                        0.0F,
                        velocity.velocityZ * pace);
                }

                return liftedPosition;
            }
        }

        const auto walkSpeed = std::sqrt(
            (velocity.velocityX * velocity.velocityX)
            + (velocity.velocityZ * velocity.velocityZ));

        const auto stoodPosition = supportingVoxel(
            filledCells,
            columnOf(position.x),
            columnOf(position.z),
            position.y);
        const auto climbing =
            stoodPosition.has_value()
            && stoodPosition->kind == voxel::Kind::Ramp;
        const auto wading =
            stoodPosition.has_value()
            && stoodPosition->kind == voxel::Kind::Water;
        const auto pace = kWalkSpeed * velocity.speedMultiplier
                          * (climbing ? kRampSpeedFactor : 1.0F)
                          * (wading ? kWaterSpeedFactor : 1.0F)
                          / std::max(walkSpeed, 1.0F);
        const auto byX = velocity.velocityX * pace;
        const auto byZ = velocity.velocityZ * pace;

        return snappedToGround(
            filledCells,
            movedBy(
                filledCells,
                movedBy(filledCells, position, byX, 0.0F),
                0.0F,
                byZ));
    }

    std::array<voxel::VoxelCell, 2> stoodCells(
        const component::Position position)
    {
        const voxel::VoxelCell standsInCell{
            .x = levelAt(position.x),
            .y = levelAt(position.y),
            .z = levelAt(position.z)};

        return {
            standsInCell,
            voxel::VoxelCell{
                .x = standsInCell.x,
                .y = standsInCell.y - 1,
                .z = standsInCell.z}};
    } // GCOVR_EXCL_LINE

}
