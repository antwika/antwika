#pragma once

#include <cstddef>
#include <optional>
#include <set>

#include <antwika/map/MapFile.hpp>
#include <antwika/voxel/VoxelCell.hpp>

namespace antwika::gameplay
{

    struct GateState final
    {
        std::size_t keysHeld = 0;

        std::set<voxel::VoxelCell> collectedKeyCells;

        std::optional<map::Placement> checkpointPlacement;

        std::optional<voxel::VoxelCell> checkpointOnCell;

        std::optional<voxel::VoxelCell> announcedDoorCell;

        std::optional<voxel::VoxelCell> lockedExitAnnouncedCell;
    };

}
