#pragma once

#include <map>

#include "antwika/voxel/VoxelMaterial.hpp"
#include "antwika/voxel/VoxelPosition.hpp"

namespace antwika::voxel
{

    using Voxels = std::map<VoxelPosition, VoxelMaterial>;

}
