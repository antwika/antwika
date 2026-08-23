#pragma once

#include <array>
#include <cstddef>
#include <vector>

#include <antwika/component/Lamplight.hpp>
#include <antwika/gfx/Camera3D.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/CubeFace.hpp>
#include <antwika/gfx/Math3D.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>

#include <antwika/voxel/VoxelPosition.hpp>
#include <antwika/voxelmap/Voxel.hpp>
#include <antwika/voxelmap/VoxelPick.hpp>
#include <antwika/collision/Collision.hpp>

namespace antwika::light
{

    inline constexpr float kLightAmbient = 0.0F;

    inline constexpr float kNightRange = 30.0F;

    inline constexpr std::size_t kMaxLamps = 8;

    inline constexpr float kSightRange = 40.0F;

    inline constexpr float kLevelFade = 0.125F;

    inline constexpr float kLampStrength = 0.55F;

    inline constexpr float kLampGizmoSize = 0.3F;

    struct Lamp final
    {
        voxel::VoxelPosition position{};

        gfx::Color tintColor = component::kLampTintColor;

        [[nodiscard]] bool operator==(const Lamp &other) const
            = default;
    };

    [[nodiscard]] gfx::Vec3 getLampPosition(Lamp lamp);

    [[nodiscard]] std::vector<Lamp> withoutLampAt(
        const std::vector<Lamp> &lamps, voxel::VoxelPosition position);

    inline constexpr std::uint32_t kShadowFaceResolution = 512;

    inline constexpr float kLampNearPlane = 0.125F * voxel::kVoxelSide;

    inline constexpr float kLampFarPlane = kSightRange;

    inline constexpr float kLampShadowBias = 0.0002F;

    [[nodiscard]] gfx::Size getShadowAtlasSize();

    [[nodiscard]] gfx::Rect getShadowFaceRect(
        std::size_t slot, gfx::CubeFace face);

    [[nodiscard]] gfx::Camera3D getShadowCamera(
        gfx::Vec3 position, gfx::CubeFace face);

    [[nodiscard]] std::array<voxelmap::LineSegment, 3> getLampGizmoSpans(
        Lamp lamp);

    [[nodiscard]] std::vector<Lamp> withLampAt(
        const std::vector<Lamp> &lamps,
        voxel::VoxelPosition position,
        gfx::Color tintColor);

}
