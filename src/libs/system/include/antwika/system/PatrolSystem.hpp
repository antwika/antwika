#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <vector>

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/geometry/Math3D.hpp>
#include <antwika/time/Tick.hpp>
#include <antwika/voxel/VoxelPosition.hpp>
#include <antwika/voxel/Voxels.hpp>

#include "antwika/system/SimulationState.hpp"

namespace antwika::system
{

    class PatrolSystem final : public ecs::ISystem
    {
    public:
        PatrolSystem(
            const voxel::Voxels &solidVoxels,
            const std::vector<std::vector<voxel::VoxelPosition>>
                &stopPositions,
            const SimulationState &simulation) noexcept;

        void forget() noexcept;

        void update(ecs::World &world, time::Tick tick) override;

    private:
        const voxel::Voxels *solidVoxels;
        const std::vector<std::vector<voxel::VoxelPosition>> *stopPositions;
        const SimulationState *simulation;
        std::map<ecs::Entity, std::vector<geometry::Vec3>> routePositions;
    };

}
