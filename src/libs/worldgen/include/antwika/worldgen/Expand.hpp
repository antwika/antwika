#pragma once

#include <antwika/voxel/Voxels.hpp>

namespace antwika::worldgen
{

    [[nodiscard]] voxel::Voxels chunkVoxels(const voxel::Voxels &cubeVoxels);

}
