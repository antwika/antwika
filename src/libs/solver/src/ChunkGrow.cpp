#include "antwika/solver/ChunkGrow.hpp"

#include <algorithm>
#include <map>
#include <utility>

#include <antwika/voxel/VoxelCube.hpp>
#include <antwika/tilemap/TileEdges.hpp>

namespace antwika::solver
{

    voxel::Voxels hintsFrom(
        const voxel::Voxels &voxels,
        const worldgen::ChunkShape shape,
        const voxel::VoxelPosition originPointPosition)
    {
        voxel::Voxels hintVoxels;

        for (const auto &[piecePosition, material] : voxels)
        {
            const auto corner = voxel::cubeCornerOf(piecePosition);
            const voxel::VoxelPosition cubePosition{
                .x = (corner.x / voxel::kCubeSide) - originPointPosition.x,
                .y = (corner.y / voxel::kCubeSide) - originPointPosition.y,
                .z = (corner.z / voxel::kCubeSide) - originPointPosition.z};

            if (!worldgen::isWithin(shape, cubePosition))
            {
                continue;
            }

            const voxel::VoxelPosition hintPosition{
                .x = cubePosition.x + originPointPosition.x,
                .y = cubePosition.y + originPointPosition.y,
                .z = cubePosition.z + originPointPosition.z};

            const auto standing = hintVoxels.find(hintPosition);

            if (standing == hintVoxels.end()
                || (standing->second.facing == voxel::Facing::Any
                    && material.facing != voxel::Facing::Any))
            {
                hintVoxels[hintPosition] = material;
            }
        }

        return hintVoxels;
    } // GCOVR_EXCL_LINE

    voxel::Voxels getWithChunkSpliced(
        voxel::Voxels pileVoxels,
        const worldgen::VoxelBox box,
        const voxel::Voxels &grownVoxels)
    {
        std::erase_if(
            pileVoxels,
            [box](const auto &standing)
            { return worldgen::holds(box, standing.first); });

        for (const auto &[position, material] : grownVoxels)
        {
            pileVoxels[position] = material;
        }

        return pileVoxels;
    } // GCOVR_EXCL_LINE

    std::string getGrowTrouble(const worldgen::ChunkResult &result)
    {
        switch (result.outcome)
        {
        case worldgen::ChunkOutcome::Grown:
            return "the block stands";
        case worldgen::ChunkOutcome::HintOutside:
            return "a painted cube stands outside the block";
        case worldgen::ChunkOutcome::HintUnknown:
            return "a painted cube is of no kind the rules know";
        case worldgen::ChunkOutcome::HintsConflict:
            return "the painted cubes stand against one another";
        case worldgen::ChunkOutcome::NoWayUp:
            return "the painted cubes wall off the climb";
        case worldgen::ChunkOutcome::Unsatisfiable:
            return "no block can be built to that hand";
        case worldgen::ChunkOutcome::LimitExceeded:
            break;
        }

        return "the growing gave up; another try may yet stand";
    }

}
