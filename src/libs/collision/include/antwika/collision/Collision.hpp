#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

#include <antwika/component/Position.hpp>
#include <antwika/component/Velocity.hpp>
#include <antwika/geometry/Math3D.hpp>

#include <antwika/voxelmap/Voxel.hpp>
#include <antwika/voxel/VoxelCube.hpp>
#include <antwika/voxel/Voxels.hpp>
#include <antwika/tilemap/TileEdges.hpp>

namespace antwika::collision
{

    inline constexpr float kWalkSpeed = 0.16F;

    inline constexpr float kRampSpeedFactor = 0.5F;

    inline constexpr float kWalkerStep = voxel::kVoxelSide;

    inline constexpr float kRampSideStep = kWalkerStep / 2.0F;

    inline constexpr std::int32_t kMaxFallDepth = 64;

    inline constexpr float kFallSpeed = kWalkSpeed * 2.0F;

    inline constexpr float kWaterSpeedFactor = 0.4F;

    inline constexpr float kRunSpeedMultiplier = 2.0F;

    inline constexpr std::int32_t kWalkerHeight = voxel::kCubeSide;

    inline constexpr float kWalkerPixel = 1.0F / 12.0F;

    inline constexpr float kFootprintSide = 1.0F * voxel::kVoxelSide;

    inline constexpr float kFootprintWidth = 8.0F * kWalkerPixel;

    inline constexpr float kFootprintDepth = 6.0F * kWalkerPixel;

    inline constexpr float kFootprintPivotY = 7.0F * kWalkerPixel;

    [[nodiscard]] std::int32_t columnOf(float coordinate);

    [[nodiscard]] geometry::Vec3 positionOf(component::Position position);

    [[nodiscard]] component::Position positionFrom(geometry::Vec3 position);

    [[nodiscard]] bool isSolid(
        const voxel::Voxels &filledVoxels, voxel::VoxelPosition position);

    [[nodiscard]] bool hasHeadroom(
        const voxel::Voxels &filledVoxels,
        voxel::VoxelPosition groundPosition);

    [[nodiscard]] std::optional<float> getGroundHeightAtColumn(
        const voxel::Voxels &filledVoxels,
        std::int32_t x,
        std::int32_t z,
        float feet);

    [[nodiscard]] std::optional<voxel::VoxelCell> getSupportingVoxel(
        const voxel::Voxels &filledVoxels,
        std::int32_t x,
        std::int32_t z,
        float feet);

    [[nodiscard]] std::optional<voxel::VoxelCell> getSupportingVoxel(
        const voxel::Voxels &filledVoxels,
        std::int32_t x,
        std::int32_t z,
        float feet,
        float stepUp);

    [[nodiscard]] float getGroundHeightOn(
        const voxel::Voxels &filledVoxels,
        voxel::VoxelCell groundCell,
        float x,
        float z);

    [[nodiscard]] std::optional<float> getGroundHeightUnderFootprint(
        const voxel::Voxels &filledVoxels,
        float x,
        float z,
        float feet);

    [[nodiscard]] std::optional<float> getGroundHeightUnderFootprint(
        const voxel::Voxels &filledVoxels,
        float x,
        float z,
        float feet,
        float stepUp);

    [[nodiscard]] std::optional<component::Position> getRestPositionOverColumn(
        const voxel::Voxels &filledVoxels,
        std::int32_t x,
        std::int32_t z);

    [[nodiscard]] std::optional<component::Position> getSpawnPosition(
        const voxel::Voxels &filledVoxels);

    [[nodiscard]] component::Position getMovedWithCollision(
        const voxel::Voxels &filledVoxels,
        component::Position position,
        component::Velocity velocity);

    [[nodiscard]] std::array<voxel::VoxelPosition, 2> getStoodCells(
        component::Position position);

}
