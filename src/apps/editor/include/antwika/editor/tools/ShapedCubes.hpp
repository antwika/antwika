#pragma once

#include <vector>

#include <antwika/map/Settings.hpp>
#include <antwika/voxel/VoxelPosition.hpp>

namespace antwika::editor
{

    [[nodiscard]] std::vector<voxel::VoxelPosition> getShapedCubes(
        voxel::VoxelPosition fromPosition,
        voxel::VoxelPosition toPosition,
        map::Paint paint);

}
