#pragma once

#include <set>
#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/input/DirectionKeys.hpp>
#include <antwika/time/Tick.hpp>
#include <antwika/voxel/VoxelPosition.hpp>
#include <antwika/voxel/Voxels.hpp>
#include <antwika/collision/Collision.hpp>

namespace antwika::system
{

    class WalkSystem final : public ecs::ISystem
    {
    public:
        explicit WalkSystem(
            const voxel::Voxels &solidVoxels) noexcept;

        void update(ecs::World &world, time::Tick tick) override;

    private:
        const voxel::Voxels *solidVoxels;
    };

}
