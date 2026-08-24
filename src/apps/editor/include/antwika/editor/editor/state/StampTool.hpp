#pragma once

#include <optional>

#include <antwika/voxel/VoxelPosition.hpp>
#include <antwika/voxel/Voxels.hpp>

namespace antwika::editor
{

    struct StampTool final
    {
        voxel::Voxels voxels;

        std::optional<voxel::VoxelPosition> fromPosition;
    };

}
