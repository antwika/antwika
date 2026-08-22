#pragma once

#include <glm/vec3.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <set>
#include <vector>

#include "antwika/voxel/VoxelCell.hpp"

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

    [[nodiscard]] std::vector<StairQuad> stairQuads(VoxelCell climbCell);

    [[nodiscard]] VoxelCell inferredRampDirection(
        const std::vector<VoxelCell> &cells, VoxelCell cell);

    [[nodiscard]] VoxelCell inferredRampDirection(
        const std::set<VoxelCell> &filledCells, VoxelCell cell);

    [[nodiscard]] Facing facingOfStep(VoxelCell climbCell);

    [[nodiscard]] VoxelCell stepVectorFor(Facing facing);

    [[nodiscard]] StairHalf stairHalfOf(
        const std::vector<VoxelCell> &cells, VoxelCell cell);

    [[nodiscard]] StairHalf stairHalfOf(
        const std::set<VoxelCell> &filledCells, VoxelCell cell);

}
