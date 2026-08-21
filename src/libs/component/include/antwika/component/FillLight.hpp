#pragma once

#include <antwika/gfx/Color.hpp>
#include <antwika/voxel/VoxelCell.hpp>

#include "antwika/component/Lamplight.hpp"

namespace antwika::component
{

    inline constexpr float kFillLightHeight = 3.5F * voxel::kVoxelSide;

    inline constexpr float kFillLightRange = 1.5F * kLampRange;

    inline constexpr gfx::Color kFillLightTintColor{
        .red = 217, .green = 201, .blue = 170, .alpha = 255};

    struct FillLight final
    {
        gfx::Color tintColor = kFillLightTintColor;
        float aboveHeight = kFillLightHeight;
        float reach = kFillLightRange;
    };

}
