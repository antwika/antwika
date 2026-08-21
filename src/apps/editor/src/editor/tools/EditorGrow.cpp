#include <string>
#include <utility>

#include <antwika/voxel/VoxelCell.hpp>
#include <antwika/voxel/VoxelCube.hpp>
#include <antwika/solver/ChunkGrow.hpp>
#include <antwika/worldgen/CityRuleset.hpp>
#include <antwika/worldgen/Expand.hpp>
#include <antwika/worldgen/Grow.hpp>
#include <antwika/worldgen/Ruleset.hpp>
#include <antwika/worldgen/ruleset/CompiledRuleset.hpp>

#include "antwika/editor/Editor.hpp"

namespace antwika::editor
{

    namespace
    {
        const worldgen::CompiledRuleset &shippedRules()
        {
            static const worldgen::CompiledRuleset compiledRuleset(
                worldgen::cityRuleset());

            return compiledRuleset;
        }
    }

    void Editor::growChunk()
    {
        const worldgen::ChunkRequest request{
            .seed = growSeed++,
            .shape = growShape,
            .originCell = voxel::VoxelCell{},
            .hintCells = solver::hintsFrom(map.voxels, growShape,
                voxel::VoxelCell{})};

        const auto result = worldgen::growChunk(shippedRules(), request);

        growTroubleCells.clear();

        if (result.outcome != worldgen::ChunkOutcome::Grown)
        {
            for (const voxel::VoxelCell cube : result.culpritCells)
            {
                growTroubleCells.push_back(
                    voxel::VoxelCell{
                        .x = cube.x * voxel::kCubeSide,
                        .y = cube.y * voxel::kCubeSide,
                        .z = cube.z * voxel::kCubeSide});
            }

            showStatus(solver::growTrouble(result), true);

            return;
        }

        pushUndo();

        map.voxels = solver::withChunkSpliced(
            std::move(map.voxels),
            worldgen::chunkBox(request.shape, request.originCell),
            worldgen::chunkVoxels(result.cubeCells));

        dirty = true;
        rebuildWorld();

        showStatus(
            "a block of " + std::to_string(result.cubeCells.size())
                + " cubes stands",
            false);
    }

}
