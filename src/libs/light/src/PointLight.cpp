#include "antwika/light/PointLight.hpp"

#include <cmath>
#include <numbers>
#include <vector>

#include <antwika/voxel/VoxelCube.hpp>

namespace antwika::light
{

    gfx::Vec3 getLampPosition(const Lamp lamp)
    {
        return voxelmap::getCubeMiddle(lamp.position);
    }

    gfx::Size getShadowAtlasSize()
    {
        return gfx::Size{
            .width = kShadowFaceResolution
                     * static_cast<std::uint32_t>(gfx::kCubeFaces),
            .height = kShadowFaceResolution
                      * static_cast<std::uint32_t>(kMaxLamps)};
    }

    gfx::Rect getShadowFaceRect(
        const std::size_t slot, const gfx::CubeFace face)
    {
        const auto side =
            static_cast<std::int32_t>(kShadowFaceResolution);

        return gfx::Rect{
            .originPoint =
                {.x = static_cast<std::int32_t>(face) * side,
                 .y = static_cast<std::int32_t>(slot) * side},
            .size =
                {.width = kShadowFaceResolution,
                 .height = kShadowFaceResolution}};
    }

    gfx::Camera3D getShadowCamera(
        const gfx::Vec3 position, const gfx::CubeFace face)
    {
        return gfx::Camera3D{
            position,
            position + gfx::directionOf(face),
            gfx::upVectorOf(face),
            gfx::Perspective{ // GCOVR_EXCL_LINE
                .fovYRadians = std::numbers::pi_v<float> / 2.0F,
                .aspectRatio = 1.0F,
                .nearPlane = kLampNearPlane,
                .farPlane = kLampFarPlane}};
    } // GCOVR_EXCL_LINE

    std::array<voxelmap::LineSegment, 3> getLampGizmoSpans(const Lamp lamp)
    {
        const auto middle = getLampPosition(lamp);
        const auto arm = kLampGizmoSize;

        return {
            voxelmap::LineSegment{
                .fromPosition = middle - gfx::Vec3{arm, 0.0F, 0.0F},
                .toPosition = middle + gfx::Vec3{arm, 0.0F, 0.0F}},
            voxelmap::LineSegment{
                .fromPosition = middle - gfx::Vec3{0.0F, arm, 0.0F},
                .toPosition = middle + gfx::Vec3{0.0F, arm, 0.0F}},
            voxelmap::LineSegment{
                .fromPosition = middle - gfx::Vec3{0.0F, 0.0F, arm},
                .toPosition = middle + gfx::Vec3{0.0F, 0.0F, arm}}};
    } // GCOVR_EXCL_LINE

    std::vector<Lamp> withoutLampAt(
        const std::vector<Lamp> &lamps, const voxel::VoxelPosition position)
    {
        std::vector<Lamp> keptLamps;

        const auto cornerPosition = voxel::cubeCornerOf(position);

        for (const auto lamp : lamps)
        {
            if (!(voxel::cubeCornerOf(lamp.position) == cornerPosition))
            {
                keptLamps.push_back(lamp);
            }
        }

        return keptLamps;
    } // GCOVR_EXCL_LINE

    std::vector<Lamp> withLampAt(
        const std::vector<Lamp> &lamps,
        const voxel::VoxelPosition position,
        const gfx::Color tintColor)
    {
        auto updatedLamps = withoutLampAt(lamps, position);

        if (updatedLamps.size() >= kMaxLamps)
        {
            return lamps;
        }

        updatedLamps.push_back(
            Lamp{.position = voxel::cubeCornerOf(position),
                 .tintColor = tintColor});

        return updatedLamps;
    } // GCOVR_EXCL_LINE

}
