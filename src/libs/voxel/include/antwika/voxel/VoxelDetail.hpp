#pragma once

#include <glm/vec3.hpp>

#include <array>
#include <cstddef>
#include <optional>

#include "antwika/voxel/Face.hpp"
#include "antwika/voxel/FaceUv.hpp"
#include "antwika/voxel/VoxelMaterial.hpp"
#include "antwika/voxel/VoxelPosition.hpp"
#include "antwika/voxel/VoxelStairs.hpp"
#include "antwika/voxel/Voxels.hpp"

namespace antwika::voxel::detail
{

    constexpr std::size_t kFaces = 6;

    constexpr float kHalf = kVoxelSide / 2.0F;

    constexpr std::array<Face, kFaces> kVoxelFaces{
        Face{
            .neighbourOffsetPosition = {.z = 1},
            .normal = {0.0F, 0.0F, 1.0F},
            .corners =
                {glm::vec3{-kHalf, -kHalf, kHalf},
                 glm::vec3{kHalf, -kHalf, kHalf},
                 glm::vec3{kHalf, kHalf, kHalf},
                 glm::vec3{-kHalf, kHalf, kHalf}}},
        Face{
            .neighbourOffsetPosition = {.z = -1},
            .normal = {0.0F, 0.0F, -1.0F},
            .corners =
                {glm::vec3{kHalf, -kHalf, -kHalf},
                 glm::vec3{-kHalf, -kHalf, -kHalf},
                 glm::vec3{-kHalf, kHalf, -kHalf},
                 glm::vec3{kHalf, kHalf, -kHalf}}},
        Face{
            .neighbourOffsetPosition = {.x = 1},
            .normal = {1.0F, 0.0F, 0.0F},
            .corners =
                {glm::vec3{kHalf, -kHalf, kHalf},
                 glm::vec3{kHalf, -kHalf, -kHalf},
                 glm::vec3{kHalf, kHalf, -kHalf},
                 glm::vec3{kHalf, kHalf, kHalf}}},
        Face{
            .neighbourOffsetPosition = {.x = -1},
            .normal = {-1.0F, 0.0F, 0.0F},
            .corners =
                {glm::vec3{-kHalf, -kHalf, -kHalf},
                 glm::vec3{-kHalf, -kHalf, kHalf},
                 glm::vec3{-kHalf, kHalf, kHalf},
                 glm::vec3{-kHalf, kHalf, -kHalf}}},
        Face{
            .neighbourOffsetPosition = {.y = 1},
            .normal = {0.0F, 1.0F, 0.0F},
            .corners =
                {glm::vec3{-kHalf, kHalf, kHalf},
                 glm::vec3{kHalf, kHalf, kHalf},
                 glm::vec3{kHalf, kHalf, -kHalf},
                 glm::vec3{-kHalf, kHalf, -kHalf}}},
        Face{
            .neighbourOffsetPosition = {.y = -1},
            .normal = {0.0F, -1.0F, 0.0F},
            .corners =
                {glm::vec3{-kHalf, -kHalf, -kHalf},
                 glm::vec3{kHalf, -kHalf, -kHalf},
                 glm::vec3{kHalf, -kHalf, kHalf},
                 glm::vec3{-kHalf, -kHalf, kHalf}}}};

    [[nodiscard]] VoxelPosition offsetBy(
        VoxelPosition fromPosition, VoxelPosition byPosition);

    [[nodiscard]] VoxelPosition opposite(VoxelPosition stepPosition);

    [[nodiscard]] std::optional<Kind> kindAt(
        const Voxels &filledVoxels, VoxelPosition position);

    [[nodiscard]] std::optional<Kind> effectiveKindAt(
        const Voxels &filledVoxels, VoxelPosition position);

    [[nodiscard]] std::optional<VoxelMaterial> materialAt(
        const Voxels &filledVoxels, VoxelPosition position);

    [[nodiscard]] bool isRampStep(
        const Voxels &filledVoxels, VoxelPosition position);

    constexpr float kStepHeightFraction =
        1.0F / static_cast<float>(kStepsPerVoxel);

    [[nodiscard]] FaceUv uvWithinFace(
        const Face &face, glm::vec3 one, glm::vec3 two);

}
