#pragma once

#include <cstdint>
#include <vector>

#include <antwika/voxel/VoxelPosition.hpp>
#include <antwika/worldgen/ChunkShape.hpp>

namespace antwika::editor
{

    struct GrowSetup final
    {
        std::uint64_t seed = 0;

        worldgen::ChunkShape shape{};

        std::vector<voxel::VoxelPosition> troublePositions;
    };

}
