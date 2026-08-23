#pragma once

#include <antwika/voxel/Voxels.hpp>

namespace antwika::worldgen
{

    [[nodiscard]] voxel::Voxels getChunkVoxels(const voxel::Voxels &cubeVoxels);

}
