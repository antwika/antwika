#pragma once

#include <cstddef>
#include <optional>

#include <antwika/voxel/VoxelPosition.hpp>

namespace antwika::editor
{

    struct PlateTool final
    {
        std::optional<std::size_t> chosenIndex;

        std::optional<voxel::VoxelPosition> lastStoodOnPosition;
    };

}
