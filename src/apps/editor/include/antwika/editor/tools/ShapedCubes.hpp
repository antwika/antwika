#pragma once

#include <vector>

#include <antwika/voxel/VoxelPosition.hpp>

#include "antwika/editor/Preferences.hpp"

namespace antwika::editor
{

    [[nodiscard]] std::vector<voxel::VoxelPosition> getShapedCubes(
        voxel::VoxelPosition fromPosition,
        voxel::VoxelPosition toPosition,
        Paint paint);

}
