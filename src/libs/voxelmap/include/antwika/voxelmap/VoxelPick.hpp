#pragma once

#include <array>
#include <cstddef>
#include <optional>
#include <span>
#include <vector>

#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/Camera3D.hpp>
#include <antwika/gfx/Math3D.hpp>
#include <antwika/gfx/PointF.hpp>
#include <antwika/gfx/Size.hpp>

#include <antwika/voxel/VoxelOcclusion.hpp>

#include <antwika/tilemap/Tilemap.hpp>
#include <antwika/voxelmap/Voxel.hpp>

#include "antwika/voxelmap/LineSegment.hpp"
#include "antwika/voxelmap/Ray.hpp"

namespace antwika::voxelmap
{

    [[nodiscard]] Ray getRayThrough(
        const gfx::Camera3D &camera,
        gfx::Size canvasSize,
        gfx::PointF point);

    inline constexpr std::int32_t kGridMarginCubes = 2;

    [[nodiscard]] std::vector<LineSegment> getLevelGridLines(
        const voxel::Voxels &voxels, std::int32_t level);

    [[nodiscard]] std::array<LineSegment, 12> getCubeWireframe(
        voxel::VoxelPosition position);

    [[nodiscard]] std::vector<LineSegment> getBuildableTopOutlines(
        const voxel::Voxels &voxels, std::int32_t level);

    [[nodiscard]] gfx::Vec3 getFaceMiddle(FaceRef face);

    [[nodiscard]] bool isFrontFacing(
        const gfx::Camera3D &camera,
        const gfx::Mat4 &modelMatrix,
        std::size_t side);

    [[nodiscard]] std::optional<gfx::PointF> getProjectToScreen(
        const gfx::Camera3D &camera,
        const gfx::Mat4 &modelMatrix,
        gfx::Size canvasSize,
        gfx::Vec3 position);

    [[nodiscard]] std::optional<gfx::PointF> getProjectToScreen(
        const gfx::Mat4 &clipMatrix, gfx::Size canvasSize, gfx::Vec3 position);

    [[nodiscard]] Ray getRayInModelSpace(
        const Ray &ray, const gfx::Mat4 &modelMatrix);

    [[nodiscard]] std::optional<gfx::Vec3> getPlaneHit(
        const Ray &ray, float height);

    [[nodiscard]] std::optional<FaceRef> getRaycastFace(
        const voxel::Voxels &voxels, const Ray &ray);

    [[nodiscard]] tilemap::Tile getFaceTile(FaceRef pickRef);

    [[nodiscard]] std::optional<voxel::VoxelPosition> getCellAtLevel(
        const Ray &ray, std::int32_t level);

    [[nodiscard]] std::optional<voxel::VoxelPosition> getCellUnder(
        const gfx::Camera3D &camera,
        const gfx::Mat4 &modelMatrix,
        gfx::Size canvasSize,
        gfx::PointF point,
        std::int32_t level);

    using voxel::kMaxOccludedVoxels;

    using voxel::getOccludingVoxels;

    using voxel::kOcclusionMaskWidth;

    using voxel::kOcclusionMaskLevels;

    [[nodiscard]] voxel::VoxelPosition getOcclusionMaskOrigin(
        voxel::VoxelPosition aboutPosition);

    [[nodiscard]] gfx::Bitmap getOcclusionMask(
        const voxel::Voxels &hiddenVoxels,
        voxel::VoxelPosition cornerPosition);

    [[nodiscard]] std::vector<LineSegment> getOccluderFootprints(
        const voxel::Voxels &hiddenVoxels);

    [[nodiscard]] std::optional<tilemap::Tile> getTilePicked(
        const voxel::Voxels &voxels,
        std::span<const tilemap::Tile> drawnTiles,
        const gfx::Camera3D &camera,
        const gfx::Mat4 &modelMatrix,
        gfx::Size canvasSize,
        gfx::PointF point);

    [[nodiscard]] std::optional<tilemap::Tile> getTilePicked(
        const voxel::Voxels &voxels,
        std::span<const FaceRef> faces,
        std::span<const tilemap::Tile> drawnTiles,
        const gfx::Camera3D &camera,
        const gfx::Mat4 &modelMatrix,
        gfx::Size canvasSize,
        gfx::PointF point);

    [[nodiscard]] std::optional<std::size_t> getFacePicked(
        const voxel::Voxels &voxels,
        std::span<const FaceRef> faces,
        const gfx::Camera3D &camera,
        const gfx::Mat4 &modelMatrix,
        gfx::Size canvasSize,
        gfx::PointF point);

}
