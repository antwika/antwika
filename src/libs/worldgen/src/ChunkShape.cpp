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

    std::size_t cubeCount(const ChunkShape shape)
    {
        requireASide(shape);

        return static_cast<std::size_t>(shape.width)
               * static_cast<std::size_t>(shape.depth)
               * static_cast<std::size_t>(shape.height);
    }

    bool within(const ChunkShape shape, const voxel::VoxelCell cubeCell)
    {
        return cubeCell.x >= 0 && cubeCell.x < shape.width && cubeCell.y >= 0
               && cubeCell.y < shape.height && cubeCell.z >= 0
               && cubeCell.z < shape.depth;
    }

    std::size_t cellOf(const ChunkShape shape, const voxel::VoxelCell cubeCell)
    {
        requireASide(shape);

        if (!within(shape, cubeCell))
        {
            throw WorldgenError("cellOf: the cube lies outside the chunk");
        }

        const auto levelIndex = static_cast<std::size_t>(cubeCell.y);
        const auto rowIndex = static_cast<std::size_t>(cubeCell.z);
        const auto columnIndex = static_cast<std::size_t>(cubeCell.x);

        return ((levelIndex * static_cast<std::size_t>(shape.depth)) + rowIndex)
                   * static_cast<std::size_t>(shape.width)
               + columnIndex;
    }

    voxel::VoxelCell cubeAt(const ChunkShape shape, const std::size_t cell)
    {
        if (cell >= cubeCount(shape))
        {
            throw WorldgenError("cubeAt: the chunk holds no such cell");
        }

        const auto width = static_cast<std::size_t>(shape.width);
        const auto depth = static_cast<std::size_t>(shape.depth);

        return voxel::VoxelCell{
            .x = static_cast<std::int32_t>(cell % width),
            .y = static_cast<std::int32_t>(cell / (width * depth)),
            .z = static_cast<std::int32_t>((cell / width) % depth)};
    }

    VoxelBox chunkBox(const ChunkShape shape, const voxel::VoxelCell originCell)
    {
        requireASide(shape);

        return VoxelBox{
            .lowCell =
                voxel::VoxelCell{
                    .x = originCell.x * voxel::kCubeSide,
                    .y = originCell.y * voxel::kCubeSide,
                    .z = originCell.z * voxel::kCubeSide},
            .highCell = voxel::VoxelCell{
                .x = (originCell.x + shape.width) * voxel::kCubeSide,
                .y = (originCell.y + shape.height) * voxel::kCubeSide,
                .z = (originCell.z + shape.depth) * voxel::kCubeSide}};
    }

    bool holds(const VoxelBox box, const voxel::VoxelCell voxel)
    {
        return voxel.x >= box.lowCell.x && voxel.x < box.highCell.x
               && voxel.y >= box.lowCell.y && voxel.y < box.highCell.y
               && voxel.z >= box.lowCell.z && voxel.z < box.highCell.z;
    }

}
