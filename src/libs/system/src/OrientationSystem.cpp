#include "antwika/system/OrientationSystem.hpp"

#include <algorithm>
#include <antwika/component/TurnIntent.hpp>
#include <antwika/component/Orientation.hpp>

namespace antwika::system
{

    OrientationSystem::OrientationSystem(
        const intent::DirectionKeys &lookKeys) noexcept
        : lookKeys(&lookKeys)
    {
    }

    void OrientationSystem::update(ecs::World &world, time::Tick)
    {
        for (const auto entity : world.view<component::Orientation>())
        {
            world.set<component::Orientation>(
                entity,
                rules::getRotatedBy(
                    world.get<component::Orientation>(entity),
                    component::TurnIntent{
                        .axisX = lookKeys->getAxisX(),
                        .axisZ = lookKeys->getAxisZ()}));
        }
    }

}
