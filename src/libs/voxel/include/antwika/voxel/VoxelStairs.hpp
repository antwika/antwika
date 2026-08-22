#pragma once

#include <glm/vec3.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <antwika/enums/Enumeration.hpp>

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

    [[nodiscard]] constexpr StairHalf lastEnumerator(StairHalf) noexcept
    {
        return StairHalf::Upper;
    }

    inline constexpr std::array<StairHalf, enums::kCount<StairHalf>>
        kEveryStairHalf = enums::kAll<StairHalf>;

    enum class StairPart : std::uint8_t
    {
        Any,
        Front,
        Side,
    };

    [[nodiscard]] constexpr StairPart lastEnumerator(StairPart) noexcept
    {
        return StairPart::Side;
    }

    inline constexpr std::array<StairPart, enums::kCount<StairPart>>
        kEveryStairPart = enums::kAll<StairPart>;

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
