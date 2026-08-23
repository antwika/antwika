#include "antwika/worldgen/ChunkShape.hpp"

#include <antwika/voxel/VoxelCube.hpp>

#include "antwika/worldgen/WorldgenError.hpp"

namespace antwika::worldgen
{

    namespace
    {
        void requireASide(const ChunkShape shape)
        {
            if (shape.width <= 0 || shape.depth <= 0 || shape.height <= 0)
            {
                throw WorldgenError(
                    "ChunkShape: every side must be greater than nought");
            }
        }
    }

    std::size_t getCubeCount(const ChunkShape shape)
    {
        requireASide(shape);

        return static_cast<std::size_t>(shape.width)
               * static_cast<std::size_t>(shape.depth)
               * static_cast<std::size_t>(shape.height);
    }

    bool isWithin(const ChunkShape shape, const voxel::VoxelPosition cubePosition)
    {
        return cubePosition.x >=
            0 && cubePosition.x < shape.width && cubePosition.y >= 0
               && cubePosition.y < shape.height && cubePosition.z >= 0
               && cubePosition.z < shape.depth;
    }

    std::size_t cellOf(const ChunkShape shape,
        const voxel::VoxelPosition cubePosition)
    {
        requireASide(shape);

        if (!isWithin(shape, cubePosition))
        {
            throw WorldgenError("cellOf: the cube lies outside the chunk");
        }

        const auto levelIndex = static_cast<std::size_t>(cubePosition.y);
        const auto rowIndex = static_cast<std::size_t>(cubePosition.z);
        const auto columnIndex = static_cast<std::size_t>(cubePosition.x);

        return ((levelIndex * static_cast<std::size_t>(shape.depth)) + rowIndex)
                   * static_cast<std::size_t>(shape.width)
               + columnIndex;
    }

    voxel::VoxelPosition cubeAt(const ChunkShape shape, const std::size_t cell)
    {
        if (cell >= getCubeCount(shape))
        {
            throw WorldgenError("cubeAt: the chunk holds no such cell");
        }

        const auto width = static_cast<std::size_t>(shape.width);
        const auto depth = static_cast<std::size_t>(shape.depth);

        return voxel::VoxelPosition{
            .x = static_cast<std::int32_t>(cell % width),
            .y = static_cast<std::int32_t>(cell / (width * depth)),
            .z = static_cast<std::int32_t>((cell / width) % depth)};
    }

    VoxelBox getChunkBox(const ChunkShape shape,
        const voxel::VoxelPosition originPosition)
    {
        requireASide(shape);

        return VoxelBox{
            .lowPosition =
                voxel::VoxelPosition{
                    .x = originPosition.x * voxel::kCubeSide,
                    .y = originPosition.y * voxel::kCubeSide,
                    .z = originPosition.z * voxel::kCubeSide},
            .highPosition = voxel::VoxelPosition{
                .x = (originPosition.x + shape.width) * voxel::kCubeSide,
                .y = (originPosition.y + shape.height) * voxel::kCubeSide,
                .z = (originPosition.z + shape.depth) * voxel::kCubeSide}};
    }

    bool holds(const VoxelBox box, const voxel::VoxelPosition voxel)
    {
        return voxel.x >= box.lowPosition.x && voxel.x < box.highPosition.x
               && voxel.y >= box.lowPosition.y && voxel.y < box.highPosition.y
               && voxel.z >= box.lowPosition.z && voxel.z < box.highPosition.z;
    }

}
