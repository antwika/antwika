#pragma once

#include <glm/vec3.hpp>

#include <cstddef>
#include <cstdint>
#include <set>

#include "antwika/voxel/VoxelCell.hpp"

namespace antwika::voxel
{

    inline constexpr std::int32_t kRoofSearchLevels = 6;

    inline constexpr std::size_t kMaxOccludedVoxels = 4096;

    inline constexpr std::uint32_t kOcclusionMaskWidth = 32;

    inline constexpr std::size_t kOcclusionMaskLevels = 32;

    inline constexpr float kLineOfSightRise = 1.0F * kVoxelSide;

    inline constexpr float kUpperSightRise =
        kLineOfSightRise + (2.0F * kVoxelSide);

    [[nodiscard]] glm::vec3 lineOfSight(glm::vec3 standing);

    [[nodiscard]] glm::vec3 upperLineOfSight(glm::vec3 standing);

    [[nodiscard]] std::set<VoxelCell> occludingVoxels(
        const std::set<VoxelCell> &filledCells, glm::vec3 standing);

}
