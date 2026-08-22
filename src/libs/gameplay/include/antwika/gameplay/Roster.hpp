#pragma once

#include <cstddef>
#include <optional>
#include <set>

#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/map/MapFile.hpp>
#include <antwika/map/PlayerProgress.hpp>
#include <antwika/voxel/VoxelPosition.hpp>
#include <antwika/voxel/Voxels.hpp>

namespace antwika::gameplay
{

    [[nodiscard]] ecs::Entity spawnRoster(
        ecs::World &world,
        const map::Map &laidMap,
        std::size_t hero,
        map::Placement stancePlacement);

    [[nodiscard]] map::Placement startingPlacement(
        const map::Map &laidMap,
        const voxel::Voxels &voxels,
        std::optional<map::Placement> checkpointPlacement);

    void spawnItems(ecs::World &world, const map::Map &laidMap);

}
