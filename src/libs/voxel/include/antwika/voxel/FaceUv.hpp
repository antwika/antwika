#pragma once

#include <glm/vec3.hpp>
#include "antwika/voxel/VoxelCell.hpp"
#include "antwika/voxel/VoxelStairs.hpp"

namespace antwika::voxel::detail
{

    struct FaceUv final
    {
        float leastU = 0.0F;
        float mostU = 0.0F;
        float leastV = 0.0F;
        float mostV = 0.0F;
        float depth = 0.0F;
    };

}
