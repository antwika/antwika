#pragma once

#include <set>
#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/input/DirectionKeys.hpp>
#include <antwika/time/Tick.hpp>
#include <antwika/voxel/VoxelCell.hpp>
#include <antwika/collision/Collision.hpp>

namespace antwika::system
{

    class WalkSystem final : public ecs::ISystem
    {
    public:
        explicit WalkSystem(
            const std::set<voxel::VoxelCell> &solidCells) noexcept;

        void update(ecs::World &world, time::Tick tick) override;

    private:
        const std::set<voxel::VoxelCell> *solidCells;
    };

}
