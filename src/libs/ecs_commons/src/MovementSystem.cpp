#include "antwika/ecs_commons/MovementSystem.hpp"

#include "antwika/ecs_commons/GridPosition.hpp"
#include "antwika/ecs_commons/Velocity.hpp"

namespace antwika::ecs_commons
{

    void MovementSystem::update(World &world, antwika::time::Tick)
    {
        for (const auto entity : world.view<GridPosition, Velocity>())
        {
            const auto position = world.get<GridPosition>(entity);
            const auto velocity = world.get<Velocity>(entity);
            world.set<GridPosition>(entity, stepBy(position, velocity));
        }
    }

} // namespace antwika::ecs_commons
