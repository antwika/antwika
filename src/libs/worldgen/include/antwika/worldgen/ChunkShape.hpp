#pragma once

#include <cstddef>
#include <cstdint>
#include <antwika/rng/IRng.hpp>
#include <antwika/voxel/VoxelCell.hpp>
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

    [[nodiscard]] std::size_t cubeCount(ChunkShape shape);

    [[nodiscard]] bool within(ChunkShape shape, voxel::VoxelCell cubeCell);

    [[nodiscard]] std::size_t cellOf(
        ChunkShape shape, voxel::VoxelCell cubeCell);

    [[nodiscard]] voxel::VoxelCell cubeAt(
        ChunkShape shape, std::size_t cell);

    [[nodiscard]] VoxelBox chunkBox(
        ChunkShape shape, voxel::VoxelCell originCell);

    [[nodiscard]] bool holds(VoxelBox box, voxel::VoxelCell voxel);

}
