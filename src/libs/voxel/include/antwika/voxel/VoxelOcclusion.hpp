#pragma once

#include <glm/vec3.hpp>

#include <cstddef>
#include <cstdint>

#include "antwika/voxel/VoxelPosition.hpp"
#include "antwika/voxel/Voxels.hpp"

namespace antwika::voxel
{

    inline constexpr std::size_t kMaxOccludedVoxels = 4096;

    inline constexpr std::uint32_t kOcclusionMaskWidth = 32;

    inline constexpr std::size_t kOcclusionMaskLevels = 32;

    inline constexpr float kLineOfSightRise = 1.0F * kVoxelSide;

    inline constexpr float kUpperSightRise =
        kLineOfSightRise + (2.0F * kVoxelSide);

    [[nodiscard]] glm::vec3 getLineOfSight(glm::vec3 standing);

    [[nodiscard]] glm::vec3 getUpperLineOfSight(glm::vec3 standing);

    [[nodiscard]] VoxelPosition getVoxelUnder(glm::vec3 point);

    [[nodiscard]] bool isCubeAbove(
        const Voxels &filledVoxels, glm::vec3 standing, float clearance);

    [[nodiscard]] Voxels getOccludingVoxels(
        const Voxels &filledVoxels, glm::vec3 standing);

}
