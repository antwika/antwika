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

    [[nodiscard]] Ray rayThrough(
        const gfx::Camera3D &camera,
        gfx::Size canvasSize,
        gfx::PointF point);

    inline constexpr std::int32_t kGridMarginCubes = 2;

    [[nodiscard]] std::vector<LineSegment> levelGridLines(
        const voxel::Voxels &voxels, std::int32_t level);

    [[nodiscard]] std::array<LineSegment, 12> cubeWireframe(
        voxel::VoxelPosition position);

    [[nodiscard]] std::vector<LineSegment> buildableTopOutlines(
        const voxel::Voxels &voxels, std::int32_t level);

    [[nodiscard]] gfx::Vec3 faceMiddle(FaceRef face);

    [[nodiscard]] bool isFrontFacing(
        const gfx::Camera3D &camera,
        const gfx::Mat4 &modelMatrix,
        std::size_t side);

    [[nodiscard]] std::optional<gfx::PointF> projectToScreen(
        const gfx::Camera3D &camera,
        const gfx::Mat4 &modelMatrix,
        gfx::Size canvasSize,
        gfx::Vec3 position);

    [[nodiscard]] std::optional<gfx::PointF> projectToScreen(
        const gfx::Mat4 &clipMatrix, gfx::Size canvasSize, gfx::Vec3 position);

    [[nodiscard]] Ray rayInModelSpace(
        const Ray &ray, const gfx::Mat4 &modelMatrix);

    [[nodiscard]] std::optional<gfx::Vec3> planeHit(
        const Ray &ray, float height);

    [[nodiscard]] std::optional<FaceRef> raycastFace(
        const voxel::Voxels &voxels, const Ray &ray);

    [[nodiscard]] tilemap::Tile faceTile(FaceRef pickRef);

    [[nodiscard]] std::optional<voxel::VoxelPosition> cellAtLevel(
        const Ray &ray, std::int32_t level);

    [[nodiscard]] std::optional<voxel::VoxelPosition> cellUnder(
        const gfx::Camera3D &camera,
        const gfx::Mat4 &modelMatrix,
        gfx::Size canvasSize,
        gfx::PointF point,
        std::int32_t level);

    using voxel::kMaxOccludedVoxels;

    using voxel::occludingVoxels;

    using voxel::kOcclusionMaskWidth;

    using voxel::kOcclusionMaskLevels;

    [[nodiscard]] voxel::VoxelPosition occlusionMaskOrigin(
        voxel::VoxelPosition aboutPosition);

    [[nodiscard]] gfx::Bitmap occlusionMask(
        const voxel::Voxels &hiddenVoxels,
        voxel::VoxelPosition cornerPosition);

    [[nodiscard]] std::vector<LineSegment> occluderFootprints(
        const voxel::Voxels &hiddenVoxels);

    [[nodiscard]] std::optional<tilemap::Tile> tilePicked(
        const voxel::Voxels &voxels,
        std::span<const tilemap::Tile> drawnTiles,
        const gfx::Camera3D &camera,
        const gfx::Mat4 &modelMatrix,
        gfx::Size canvasSize,
        gfx::PointF point);

    [[nodiscard]] std::optional<tilemap::Tile> tilePicked(
        const voxel::Voxels &voxels,
        std::span<const FaceRef> faces,
        std::span<const tilemap::Tile> drawnTiles,
        const gfx::Camera3D &camera,
        const gfx::Mat4 &modelMatrix,
        gfx::Size canvasSize,
        gfx::PointF point);

    [[nodiscard]] std::optional<std::size_t> facePicked(
        const voxel::Voxels &voxels,
        std::span<const FaceRef> faces,
        const gfx::Camera3D &camera,
        const gfx::Mat4 &modelMatrix,
        gfx::Size canvasSize,
        gfx::PointF point);

}
