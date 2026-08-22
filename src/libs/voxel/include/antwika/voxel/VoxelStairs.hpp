#pragma once

#include <glm/vec3.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "antwika/voxel/VoxelMaterial.hpp"
#include "antwika/voxel/VoxelPosition.hpp"
#include "antwika/voxel/Voxels.hpp"

namespace antwika::voxel
{

    enum class StairHalf : std::uint8_t
    {
        Any,
        Lower,
        Upper,
    };

    inline constexpr std::array<StairHalf, 3> kEveryStairHalf{
        StairHalf::Any, StairHalf::Lower, StairHalf::Upper};

    enum class StairPart : std::uint8_t
    {
        Any,
        Front,
        Side,
    };

    inline constexpr std::array<StairPart, 3> kEveryStairPart{
        StairPart::Any, StairPart::Front, StairPart::Side};

    inline constexpr std::size_t kStepsPerVoxel = 3;

    inline constexpr std::size_t kStairQuads =
        (kStepsPerVoxel * 4) - 2 + 2;

    struct StairQuad final
    {
        std::size_t side = 0;

        std::array<glm::vec3, 4> corners{};

        [[nodiscard]] bool operator==(const StairQuad &other) const
            = default;
    };

    [[nodiscard]] std::vector<StairQuad> stairQuads(
        VoxelPosition climbPosition);

    [[nodiscard]] VoxelPosition inferredRampDirection(
        const Voxels &filledVoxels, VoxelPosition position);

    [[nodiscard]] Facing facingOfStep(VoxelPosition climbPosition);

    [[nodiscard]] VoxelPosition stepVectorFor(Facing facing);

    [[nodiscard]] StairHalf stairHalfOf(
        const Voxels &filledVoxels, VoxelPosition position);

}
