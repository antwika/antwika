#pragma once

#include <cstddef>
#include <optional>
#include <vector>

#include <antwika/component/Lamplight.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Math3D.hpp>
#include <antwika/voxel/VoxelPosition.hpp>
#include <antwika/collision/Collision.hpp>

#include <antwika/light/PointLight.hpp>

namespace antwika::light
{

    inline constexpr float kShadowRedrawDistance =
        0.25F * voxel::kVoxelSide;

    inline constexpr float kSightClearance =
        kShadowRedrawDistance + kLampNearPlane;

    struct ActiveLight final
    {
        gfx::Vec3 position{};
        gfx::Color tintColor = component::kLampTintColor;

        float reach = component::kLampRange;

        bool castsShadows = true;

        [[nodiscard]] bool operator==(
            const ActiveLight &other) const = default;
    };

    [[nodiscard]] std::vector<ActiveLight> activeLights(
        const ecs::World &world, const std::vector<Lamp> &lamps);

    [[nodiscard]] std::vector<ActiveLight> activeLights(
        const std::vector<Lamp> &lamps);

    [[nodiscard]] std::vector<ActiveLight> activeLights(
        const ecs::World &world,
        const std::vector<ActiveLight> &folkLights,
        const std::vector<Lamp> &lamps);

    [[nodiscard]] std::vector<ActiveLight> activeLights(
        const std::vector<ActiveLight> &folkLights,
        const std::vector<Lamp> &lamps);

    [[nodiscard]] std::optional<std::size_t> carriedLightSlot(
        const ecs::World &world, ecs::Entity entity);

    [[nodiscard]] std::vector<std::size_t> dirtyShadowSlots(
        const std::vector<ActiveLight> &bakedLight,
        const std::vector<ActiveLight> &currentLight);

}
