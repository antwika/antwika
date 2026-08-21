#pragma once

#include <glm/vec3.hpp>

#include <array>
#include <cstddef>
#include <optional>
#include <set>

#include "antwika/voxel/Face.hpp"
#include "antwika/voxel/FaceUv.hpp"
#include "antwika/voxel/VoxelCell.hpp"
#include "antwika/voxel/VoxelStairs.hpp"

namespace antwika::voxel::detail
{

    constexpr std::size_t kFaces = 6;

    constexpr float kHalf = kVoxelSide / 2.0F;

    constexpr std::array<Face, kFaces> kVoxelFaces{
        Face{
            .neighbourOffsetCell = {.z = 1},
            .normal = {0.0F, 0.0F, 1.0F},
            .corners =
                {glm::vec3{-kHalf, -kHalf, kHalf},
                 glm::vec3{kHalf, -kHalf, kHalf},
                 glm::vec3{kHalf, kHalf, kHalf},
                 glm::vec3{-kHalf, kHalf, kHalf}}},
        Face{
            .neighbourOffsetCell = {.z = -1},
            .normal = {0.0F, 0.0F, -1.0F},
            .corners =
                {glm::vec3{kHalf, -kHalf, -kHalf},
                 glm::vec3{-kHalf, -kHalf, -kHalf},
                 glm::vec3{-kHalf, kHalf, -kHalf},
                 glm::vec3{kHalf, kHalf, -kHalf}}},
        Face{
            .neighbourOffsetCell = {.x = 1},
            .normal = {1.0F, 0.0F, 0.0F},
            .corners =
                {glm::vec3{kHalf, -kHalf, kHalf},
                 glm::vec3{kHalf, -kHalf, -kHalf},
                 glm::vec3{kHalf, kHalf, -kHalf},
                 glm::vec3{kHalf, kHalf, kHalf}}},
        Face{
            .neighbourOffsetCell = {.x = -1},
            .normal = {-1.0F, 0.0F, 0.0F},
            .corners =
                {glm::vec3{-kHalf, -kHalf, -kHalf},
                 glm::vec3{-kHalf, -kHalf, kHalf},
                 glm::vec3{-kHalf, kHalf, kHalf},
                 glm::vec3{-kHalf, kHalf, -kHalf}}},
        Face{
            .neighbourOffsetCell = {.y = 1},
            .normal = {0.0F, 1.0F, 0.0F},
            .corners =
                {glm::vec3{-kHalf, kHalf, kHalf},
                 glm::vec3{kHalf, kHalf, kHalf},
                 glm::vec3{kHalf, kHalf, -kHalf},
                 glm::vec3{-kHalf, kHalf, -kHalf}}},
        Face{
            .neighbourOffsetCell = {.y = -1},
            .normal = {0.0F, -1.0F, 0.0F},
            .corners =
                {glm::vec3{-kHalf, -kHalf, -kHalf},
                 glm::vec3{kHalf, -kHalf, -kHalf},
                 glm::vec3{kHalf, -kHalf, kHalf},
                 glm::vec3{-kHalf, -kHalf, kHalf}}}};

        [[nodiscard]] VoxelCell offsetBy(
            const VoxelCell fromCell, const VoxelCell byCell);

        [[nodiscard]] std::optional<Kind> kindAt(
            const std::set<VoxelCell> &filledCells, const VoxelCell cell);

        [[nodiscard]] std::optional<Kind> effectiveKindAt(
            const std::set<VoxelCell> &filledCells, VoxelCell cell);

        [[nodiscard]] std::optional<VoxelCell> voxelAt(
            const std::set<VoxelCell> &filledCells, const VoxelCell cell);

        [[nodiscard]] bool isRampStep(
            const std::set<VoxelCell> &filledCells, const VoxelCell cell);

    constexpr float kStepHeightFraction =
        1.0F / static_cast<float>(kStepsPerVoxel);

    [[nodiscard]] FaceUv uvWithinFace(
        const Face &face, const glm::vec3 one, const glm::vec3 two);

}
