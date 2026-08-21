#pragma once

#include <cstdint>

#include <antwika/gfx/Camera3D.hpp>
#include <antwika/gfx/Math3D.hpp>
#include <antwika/gfx/Size.hpp>

#include <antwika/voxel/VoxelCube.hpp>
#include <antwika/tilemap/TileEdges.hpp>

namespace antwika::camera
{

    inline constexpr float kCameraStep = 0.06F;

    inline constexpr gfx::Size kCanvasSize{.width = 480, .height = 270};

    inline constexpr std::int32_t kVoxelPixels = 15;

    inline constexpr std::int32_t kZoomStep = kVoxelPixels / 3;

    inline constexpr std::int32_t kDefaultZoom = kVoxelPixels;

    inline constexpr std::int32_t kMinZoom = kZoomStep;

    inline constexpr std::int32_t kMaxZoom = kVoxelPixels * 8;

    [[nodiscard]] float isometricPitch();

    [[nodiscard]] float orthoHalfHeight(
        gfx::Size canvasSize, std::int32_t pixelsPerVoxel);

    inline constexpr float kMouseTurn = 0.006F;

    inline constexpr float kMaxPitch = 1.5533F;

    struct CameraTransform final
    {
        gfx::Vec3 position{0.0F, 0.0F, 0.0F};

        float yaw = 0.0F;

        float pitch = 0.0F;

        [[nodiscard]] bool operator==(const CameraTransform &other) const
            = default;
    };

    [[nodiscard]] CameraTransform defaultTransform();

    [[nodiscard]] CameraTransform resetToIsometric(CameraTransform transform);

    [[nodiscard]] CameraTransform snappedPitch(CameraTransform transform);

    [[nodiscard]] CameraTransform centeredOn(
        CameraTransform transform, gfx::Vec3 position);

    [[nodiscard]] gfx::Vec3 forward(const CameraTransform &transform);

    [[nodiscard]] gfx::Vec3 forwardOnGround(const CameraTransform &transform);

    [[nodiscard]] CameraTransform rotated(
        CameraTransform transform, float byYaw, float byPitch);

    [[nodiscard]] CameraTransform movedOnGround(
        CameraTransform transform, float ahead, float acrossDistance,
        float step);

    inline constexpr float kPanStep = 0.01F;

    [[nodiscard]] CameraTransform panned(
        CameraTransform transform, float acrossDistance, float upDistance,
        float step);

    [[nodiscard]] gfx::Camera3D cameraOf(
        const CameraTransform &transform,
        gfx::Size canvasSize,
        float halfHeight);

    inline constexpr float kEditorFov = 0.6981F;

    [[nodiscard]] gfx::Camera3D perspectiveOf(
        const CameraTransform &transform,
        gfx::Size canvasSize,
        float halfHeight);

    inline constexpr float kFlyStep = 0.22F;

    inline constexpr float kFlyBoost = 3.0F;

    inline constexpr float kZoomLerpRate = 0.25F;

    [[nodiscard]] CameraTransform movedAlongView(
        CameraTransform transform,
        float ahead,
        float acrossDistance,
        float rise,
        float step);

}
