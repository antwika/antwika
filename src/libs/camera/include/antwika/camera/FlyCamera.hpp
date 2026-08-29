#pragma once

#include <cstdint>

#include <antwika/gfx/Camera3D.hpp>
#include <antwika/gfx/Math3D.hpp>
#include <antwika/gfx/Size.hpp>

namespace antwika::camera
{

    inline constexpr float kCameraStep = 0.06F;

    inline constexpr gfx::Size kCanvasSize{.width = 480, .height = 270};

    inline constexpr std::int32_t kVoxelPixels = 15;

    inline constexpr std::int32_t kZoomStep = kVoxelPixels / 3;

    inline constexpr std::int32_t kDefaultZoom = kVoxelPixels;

    inline constexpr std::int32_t kMinZoom = kZoomStep;

    inline constexpr std::int32_t kMaxZoom = kVoxelPixels * 8;

    [[nodiscard]] float getIsometricPitch();

    [[nodiscard]] float getOrthoHalfHeight(
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

    [[nodiscard]] CameraTransform getDefaultTransform();

    [[nodiscard]] CameraTransform getResetToIsometric(CameraTransform transform);

    [[nodiscard]] CameraTransform getSnappedPitch(CameraTransform transform);

    [[nodiscard]] CameraTransform getAimedAt(
        CameraTransform transform, gfx::Vec3 position);

    [[nodiscard]] CameraTransform getCenteredOn(
        CameraTransform transform, gfx::Vec3 position);

    [[nodiscard]] gfx::Vec3 getForward(const CameraTransform &transform);

    [[nodiscard]] gfx::Vec3 getForwardOnGround(const CameraTransform &transform);

    [[nodiscard]] CameraTransform getRotatedTransform(
        CameraTransform transform, float byYaw, float byPitch);

    [[nodiscard]] CameraTransform getMovedOnGround(
        CameraTransform transform, float ahead, float acrossDistance,
        float step);

    inline constexpr float kPanStep = 0.01F;

    [[nodiscard]] CameraTransform getPannedTransform(
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

    [[nodiscard]] CameraTransform getMovedAlongView(
        CameraTransform transform,
        float ahead,
        float acrossDistance,
        float rise,
        float step);

}
