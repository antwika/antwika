#pragma once

#include <compare>

#include "antwika/voxel/VoxelMaterial.hpp"
#include "antwika/voxel/VoxelPosition.hpp"

namespace antwika::voxel
{

    struct VoxelCell final
    {
        VoxelPosition position{};

        VoxelMaterial material{};

        [[nodiscard]] bool operator==(const VoxelCell &other) const
            = default;

        [[nodiscard]] auto operator<=>(const VoxelCell &other) const
            = default;
    };

}
