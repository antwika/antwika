#pragma once

#include <initializer_list>
#include <map>

#include "antwika/voxel/VoxelCell.hpp"
#include "antwika/voxel/VoxelMaterial.hpp"
#include "antwika/voxel/VoxelPosition.hpp"

namespace antwika::voxel
{

    using Voxels = std::map<VoxelPosition, VoxelMaterial>;

    [[nodiscard]] inline Voxels voxelsOf(
        const std::initializer_list<VoxelCell> cells)
    {
        Voxels voxels;

        for (const auto cell : cells)
        {
            voxels[cell.position()] = cell.material();
        }

        return voxels;
    }

}
