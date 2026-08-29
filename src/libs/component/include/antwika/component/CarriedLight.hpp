#pragma once

#include <antwika/gfx/Color.hpp>
#include <antwika/voxel/VoxelCell.hpp>

#include "antwika/component/Lamplight.hpp"

namespace antwika::component
{

    inline constexpr float kCarriedLightHeight = 1.5F * voxel::kVoxelSide;

    inline constexpr float kCarriedLightRange = 2.0F * kLampRange;

    struct CarriedLight final
    {
        gfx::Color tintColor = kLampTintColor;
        float aboveHeight = kCarriedLightHeight;
        float reach = kCarriedLightRange;

        [[nodiscard]] bool operator==(
            const CarriedLight &other) const = default;
    };

}
