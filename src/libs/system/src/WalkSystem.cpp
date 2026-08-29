#include "antwika/system/WalkSystem.hpp"

#include <antwika/component/Position.hpp>
#include <antwika/component/Velocity.hpp>

namespace antwika::system
{

    WalkSystem::WalkSystem(
        const voxel::Voxels &solidVoxels) noexcept
        : solidVoxels(&solidVoxels)
    {
    }

    void WalkSystem::update(ecs::World &world, time::Tick)
    {
        for (const auto entity :
             world.view<component::Position, component::Velocity>())
        {
            world.set<component::Position>(
                entity,
                collision::getMovedWithCollision(
                    *solidVoxels,
                    world.get<component::Position>(entity),
                    world.get<component::Velocity>(entity)));
        }
    }

}
