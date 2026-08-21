#include "antwika/camera/FlyCamera.hpp"

#include <glm/geometric.hpp>

#include <algorithm>
#include <cmath>

namespace antwika::camera
{

    namespace
    {
        constexpr float kOpeningReach = 4.0F;

        constexpr float kPitchSnapWithin = 0.001F;

        constexpr float kFarPlane = 64.0F;

        constexpr float kNearPlane = -kFarPlane;
    }

    float isometricPitch()
    {
        return -std::atan(4.0F / 3.0F);
    }

    CameraTransform snappedPitch(CameraTransform transform)
    {
        if (std::abs(transform.pitch - isometricPitch()) < kPitchSnapWithin)
        {
            transform.pitch = isometricPitch();
        }

        return transform;
    }

    float orthoHalfHeight(
        const gfx::Size canvasSize,
        const std::int32_t pixelsPerVoxel)
    {
        return static_cast<float>(canvasSize.height)
               / (2.0F * static_cast<float>(pixelsPerVoxel));
    }

    CameraTransform defaultTransform()
    {
        const auto dropping = std::sin(-isometricPitch());
        const auto ahead = std::cos(-isometricPitch());

        return CameraTransform{
            .position =
                gfx::Vec3{
                    0.0F,
                    dropping * kOpeningReach,
                    ahead * kOpeningReach},
            .yaw = 0.0F,
            .pitch = isometricPitch()};
    }

    CameraTransform resetToIsometric(CameraTransform transform)
    {
        transform.yaw = 0.0F;
        transform.pitch = isometricPitch();

        return transform;
    }

    CameraTransform centeredOn(
        CameraTransform transform, const gfx::Vec3 position)
    {
        transform.position = defaultTransform().position + position;

        return transform;
    }

    gfx::Vec3 forward(const CameraTransform &transform)
    {
        const auto level = std::cos(transform.pitch);

        return gfx::Vec3{
            std::sin(transform.yaw) * level,
            std::sin(transform.pitch),
            -std::cos(transform.yaw) * level};
    }

    gfx::Vec3 forwardOnGround(const CameraTransform &transform)
    {
        return gfx::Vec3{
            std::sin(transform.yaw), 0.0F, -std::cos(transform.yaw)};
    }

    CameraTransform rotated(
        CameraTransform transform, const float byYaw, const float byPitch)
    {
        transform.yaw += byYaw;
        transform.pitch = std::clamp(
            transform.pitch + byPitch, -kMaxPitch, kMaxPitch);

        return transform;
    }

    CameraTransform panned(
        CameraTransform transform,
        const float acrossDistance,
        const float upDistance,
        const float step)
    {
        const auto ahead = forward(transform);
        const auto right = glm::normalize(
            glm::cross(ahead, gfx::Vec3{0.0F, 1.0F, 0.0F}));
        const auto overhead = glm::cross(right, ahead);

        transform.position += (
            (right * acrossDistance) + (overhead * upDistance)) * step;

        return transform;
    }

    CameraTransform movedOnGround(
        CameraTransform transform,
        const float ahead,
        const float acrossDistance,
        const float step)
    {
        const auto askedVector =
            (forwardOnGround(transform) * ahead)
            + (gfx::Vec3{
                   std::cos(transform.yaw), 0.0F, std::sin(transform.yaw)}
               * acrossDistance);
        const auto askedLength = glm::length(askedVector);

        if (askedLength <= 0.0F)
        {
            return transform;
        }

        transform.position += (askedVector / askedLength) * step;

        return transform;
    }

    gfx::Camera3D cameraOf(
        const CameraTransform &transform,
        const gfx::Size canvasSize,
        const float halfHeight)
    {
        const auto halfWidth =
            halfHeight * static_cast<float>(canvasSize.width)
            / static_cast<float>(canvasSize.height);
        return gfx::Camera3D{
            transform.position,
            transform.position + forward(transform),
            gfx::Vec3{0.0F, 1.0F, 0.0F},
            gfx::Orthographic{ // GCOVR_EXCL_LINE
                .halfWidth = halfWidth,
                .halfHeight = halfHeight,
                .nearPlane = kNearPlane,
                .farPlane = kFarPlane}};
    } // GCOVR_EXCL_LINE

    gfx::Camera3D perspectiveOf(
        const CameraTransform &transform,
        const gfx::Size canvasSize,
        const float halfHeight)
    {
        const auto ahead = forward(transform);
        const auto backDistance =
            halfHeight / std::tan(kEditorFov / 2.0F);

        return gfx::Camera3D{
            transform.position - (ahead * backDistance),
            transform.position,
            gfx::Vec3{0.0F, 1.0F, 0.0F},
            gfx::Perspective{ // GCOVR_EXCL_LINE
                .fovYRadians = kEditorFov,
                .aspectRatio =
                    static_cast<float>(canvasSize.width)
                    / static_cast<float>(canvasSize.height),
                .nearPlane = 0.5F,
                .farPlane = backDistance + (2.0F * kFarPlane)}};
    } // GCOVR_EXCL_LINE

    CameraTransform movedAlongView(
        CameraTransform transform,
        const float ahead,
        const float acrossDistance,
        const float rise,
        const float step)
    {
        const auto forwardDirection = forward(transform);
        const auto right = glm::normalize(
            glm::cross(forwardDirection, gfx::Vec3{0.0F, 1.0F, 0.0F}));
        const auto overhead =
            glm::normalize(glm::cross(right, forwardDirection));
        const auto askedVector = (forwardDirection * ahead)
                           + (right * acrossDistance)
                           + (overhead * rise);
        const auto askedLength = glm::length(askedVector);

        if (askedLength < 0.0001F)
        {
            return transform;
        }

        transform.position += (askedVector / askedLength) * step;

        return transform;
    }

}
