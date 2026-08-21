#pragma once

#include <glm/vec3.hpp>

#include <array>
#include <cstddef>

#include "antwika/voxel/VoxelCell.hpp"
#include "antwika/voxel/VoxelStairs.hpp"

namespace antwika::voxel::detail
{

    constexpr std::size_t kCornersPerFace = 4;

    struct Face final
    {
        VoxelCell neighbourOffsetCell;
        glm::vec3 normal;
        std::array<glm::vec3, kCornersPerFace> corners;
    };

}
