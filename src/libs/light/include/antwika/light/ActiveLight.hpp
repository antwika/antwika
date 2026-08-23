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

    inline constexpr float kWalkerLight = 0.5F;

    inline constexpr float kWalkerLightRange =
        0.5F * component::kLampRange;

    struct ActiveLight final
    {
        gfx::Vec3 position{};
        gfx::Color tintColor = component::kLampTintColor;

        float reach = component::kLampRange;

        float brightness = 1.0F;

        bool castsShadows = true;

        [[nodiscard]] bool operator==(
            const ActiveLight &other) const = default;
    };

    [[nodiscard]] std::vector<ActiveLight> getActiveLights(
        const ecs::World &world, const std::vector<Lamp> &lamps);

    [[nodiscard]] std::vector<ActiveLight> getActiveLights(
        const std::vector<Lamp> &lamps);

    [[nodiscard]] std::vector<ActiveLight> getActiveLights(
        const ecs::World &world,
        const std::vector<ActiveLight> &folkLights,
        const std::vector<Lamp> &lamps);

    [[nodiscard]] std::vector<ActiveLight> getActiveLights(
        const std::vector<ActiveLight> &folkLights,
        const std::vector<Lamp> &lamps);

    [[nodiscard]] std::optional<std::size_t> getCarriedLightSlot(
        const ecs::World &world, ecs::Entity entity);

    [[nodiscard]] std::vector<std::size_t> getDirtyShadowSlots(
        const std::vector<ActiveLight> &bakedLight,
        const std::vector<ActiveLight> &currentLight);

}
