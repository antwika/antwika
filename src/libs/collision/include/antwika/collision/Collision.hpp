#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <set>
#include <vector>

#include <antwika/component/Position.hpp>
#include <antwika/component/Velocity.hpp>
#include <antwika/gfx/Math3D.hpp>

#include <antwika/voxelmap/Voxel.hpp>
#include <antwika/voxel/VoxelCube.hpp>
#include <antwika/tilemap/TileEdges.hpp>

namespace antwika::collision
{

    inline constexpr float kWalkSpeed = 0.16F;

    inline constexpr float kRampSpeedFactor = 0.5F;

    inline constexpr float kWalkerStep = voxel::kVoxelSide;

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

    [[nodiscard]] gfx::Vec3 positionOf(component::Position position);

    [[nodiscard]] component::Position positionFrom(gfx::Vec3 position);

    [[nodiscard]] bool isSolid(
        const std::set<voxel::VoxelCell> &filledCells, voxel::VoxelCell cells);

    [[nodiscard]] bool hasHeadroom(
        const std::set<voxel::VoxelCell> &filledCells,
        voxel::VoxelCell groundCell);

    [[nodiscard]] std::optional<float> groundHeightAtColumn(
        const std::set<voxel::VoxelCell> &filledCells,
        std::int32_t x,
        std::int32_t z,
        float feet);

    [[nodiscard]] std::optional<voxel::VoxelCell> supportingVoxel(
        const std::set<voxel::VoxelCell> &filledCells,
        std::int32_t x,
        std::int32_t z,
        float feet);

    [[nodiscard]] float groundHeightOn(
        const std::set<voxel::VoxelCell> &filledCells,
        voxel::VoxelCell groundCell,
        float x,
        float z);

    [[nodiscard]] std::optional<float> groundHeightUnderFootprint(
        const std::set<voxel::VoxelCell> &filledCells,
        float x,
        float z,
        float feet);

    [[nodiscard]] std::optional<component::Position> restPositionOverColumn(
        const std::set<voxel::VoxelCell> &filledCells,
        std::int32_t x,
        std::int32_t z);

    [[nodiscard]] std::optional<component::Position> spawnPosition(
        const std::vector<voxel::VoxelCell> &cells);

    [[nodiscard]] component::Position movedWithCollision(
        const std::set<voxel::VoxelCell> &filledCells,
        component::Position position,
        component::Velocity velocity);

    [[nodiscard]] std::array<voxel::VoxelCell, 2> stoodCells(
        component::Position position);

}
