#pragma once

#include <cstddef>
#include <optional>
#include <set>

#include <antwika/map/MapFile.hpp>
#include <antwika/voxel/VoxelPosition.hpp>
#include <antwika/voxel/Voxels.hpp>

namespace antwika::gameplay
{

    struct GateState final
    {
        std::size_t keysHeld = 0;

        std::set<voxel::VoxelPosition> collectedKeyPositions;

        std::optional<map::Placement> checkpointPlacement;

        std::optional<voxel::VoxelPosition> checkpointOnPosition;

        std::optional<voxel::VoxelPosition> announcedDoorPosition;

        std::optional<voxel::VoxelPosition> lockedExitAnnouncedPosition;
    };

}
