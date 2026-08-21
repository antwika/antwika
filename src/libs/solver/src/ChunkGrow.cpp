#include "antwika/solver/ChunkGrow.hpp"

#include <algorithm>
#include <map>
#include <utility>

#include <antwika/voxel/VoxelCube.hpp>
#include <antwika/voxel/VoxelCube.hpp>
#include <antwika/tilemap/TileEdges.hpp>

namespace antwika::solver
{

    std::vector<voxel::VoxelCell> hintsFrom(
        const std::vector<voxel::VoxelCell> &voxels,
        const worldgen::ChunkShape shape,
        const voxel::VoxelCell originPointCell)
    {
        std::map<voxel::VoxelCell, voxel::VoxelCell> cubeCells;

        for (const voxel::VoxelCell piece : voxels)
        {
            const auto corner = voxel::cubeCornerOf(piece);
            const voxel::VoxelCell cubeCell{
                .x = (corner.x / voxel::kCubeSide) - originPointCell.x,
                .y = (corner.y / voxel::kCubeSide) - originPointCell.y,
                .z = (corner.z / voxel::kCubeSide) - originPointCell.z};

            if (!worldgen::within(shape, cubeCell))
            {
                continue;
            }

            const voxel::VoxelCell hintCell{
                .x = cubeCell.x + originPointCell.x,
                .y = cubeCell.y + originPointCell.y,
                .z = cubeCell.z + originPointCell.z,
                .kind = piece.kind,
                .facing = piece.facing};

            const auto standing = cubeCells.find(hintCell);

            if (standing == cubeCells.end()
                || (standing->second.facing == voxel::Facing::Any
                    && piece.facing != voxel::Facing::Any))
            {
                cubeCells[hintCell] = hintCell;
            }
        }

        std::vector<voxel::VoxelCell> hintCells;
        hintCells.reserve(cubeCells.size());

        for (const auto &[where, hintCell] : cubeCells)
        {
            hintCells.push_back(hintCell);
        }

        return hintCells;
    } // GCOVR_EXCL_LINE

    std::vector<voxel::VoxelCell> withChunkSpliced(
        std::vector<voxel::VoxelCell> pileCell,
        const worldgen::VoxelBox box,
        const std::vector<voxel::VoxelCell> &grownCell)
    {
        std::erase_if(
            pileCell,
            [box](const voxel::VoxelCell voxel)
            { return worldgen::holds(box, voxel); });

        pileCell.insert(pileCell.end(), grownCell.begin(), grownCell.end());

        return pileCell;
    } // GCOVR_EXCL_LINE

    std::string growTrouble(const worldgen::ChunkResult &result)
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
