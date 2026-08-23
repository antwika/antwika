#pragma once

#include <cstddef>
#include <cstdint>
#include <antwika/rng/IRng.hpp>
#include <antwika/voxel/VoxelPosition.hpp>
#include <antwika/wfc/CompatibilityTable.hpp>
#include "antwika/worldgen/VoxelBox.hpp"

namespace antwika::worldgen
{

    inline constexpr std::int32_t kChunkWidth = 16;

    inline constexpr std::int32_t kChunkDepth = 16;

    inline constexpr std::int32_t kChunkHeight = 64;

    struct ChunkShape final
    {
        std::int32_t width = kChunkWidth;

        std::int32_t depth = kChunkDepth;

        std::int32_t height = kChunkHeight;

        [[nodiscard]] bool operator==(const ChunkShape &other) const
            = default;
    };

    [[nodiscard]] std::size_t getCubeCount(ChunkShape shape);

    [[nodiscard]] bool isWithin(ChunkShape shape,
        voxel::VoxelPosition cubePosition);

    [[nodiscard]] std::size_t cellOf(
        ChunkShape shape, voxel::VoxelPosition cubePosition);

    [[nodiscard]] voxel::VoxelPosition cubeAt(
        ChunkShape shape, std::size_t cell);

    [[nodiscard]] VoxelBox getChunkBox(
        ChunkShape shape, voxel::VoxelPosition originPosition);

    [[nodiscard]] bool holds(VoxelBox box, voxel::VoxelPosition position);

}
