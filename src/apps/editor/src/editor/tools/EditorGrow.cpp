#include <string>
#include <utility>

#include <antwika/voxel/VoxelPosition.hpp>
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
            .originPosition = voxel::VoxelPosition{},
            .hintVoxels = solver::hintsFrom(document.map.voxels, growShape,
                voxel::VoxelPosition{})};

        const auto result = worldgen::growChunk(shippedRules(), request);

        growTroublePositions.clear();

        if (result.outcome != worldgen::ChunkOutcome::Grown)
        {
            for (const voxel::VoxelPosition cube : result.culpritPositions)
            {
                growTroublePositions.push_back(
                    voxel::VoxelPosition{
                        .x = cube.x * voxel::kCubeSide,
                        .y = cube.y * voxel::kCubeSide,
                        .z = cube.z * voxel::kCubeSide});
            }

            showStatus(solver::growTrouble(result), true);

            return;
        }

        pushUndo();

        document.map.voxels = solver::withChunkSpliced(
            std::move(document.map.voxels),
            worldgen::chunkBox(request.shape, request.originPosition),
            worldgen::chunkVoxels(result.cubeVoxels));

        document.markDirty();
        rebuildWorld();

        showStatus(
            "a block of " + std::to_string(result.cubeVoxels.size())
                + " cubes stands",
            false);
    }

}
