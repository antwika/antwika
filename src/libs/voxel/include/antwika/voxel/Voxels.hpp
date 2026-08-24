#pragma once

#include <algorithm>
#include <initializer_list>
#include <unordered_map>
#include <vector>

#include "antwika/voxel/VoxelCell.hpp"
#include "antwika/voxel/VoxelMaterial.hpp"
#include "antwika/voxel/VoxelPosition.hpp"
#include "antwika/voxel/VoxelPositionHash.hpp"

namespace antwika::voxel
{

    using Voxels =
        std::unordered_map<VoxelPosition, VoxelMaterial, VoxelPositionHash>;

    [[nodiscard]] inline std::vector<VoxelCell> getSortedCells(
        const Voxels &voxels)
    {
        std::vector<VoxelCell> cells;

        cells.reserve(voxels.size());

        for (const auto &[position, material] : voxels)
        {
            cells.push_back(
                VoxelCell{.position = position, .material = material});
        }

        std::sort(
            cells.begin(),
            cells.end(),
            [](const VoxelCell &one, const VoxelCell &other)
            { return one.position < other.position; });

        return cells;
    }

    [[nodiscard]] inline Voxels voxelsOf(
        const std::initializer_list<VoxelCell> cells)
    {
        Voxels voxels;

        for (const auto cell : cells)
        {
            voxels[cell.position] = cell.material;
        }

        return voxels;
    }

}
