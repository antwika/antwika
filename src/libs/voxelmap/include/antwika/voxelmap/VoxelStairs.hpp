#pragma once

#include <antwika/gfx/RectF.hpp>
#include <antwika/voxel/VoxelStairs.hpp>

namespace antwika::voxelmap
{

    [[nodiscard]] gfx::RectF stairUvRect(
        gfx::RectF tileRect,
        const voxel::StairQuad &quad,
        bool mirrored = false);

}
