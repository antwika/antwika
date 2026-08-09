#include "antwika/game/Store.hpp"

namespace antwika::game
{

    std::int32_t stockOf(
        const antwika::ecs::World &world,
        antwika::ecs::Entity entity,
        Resource resource)
    {
        if (!world.has<Building>(entity))
        {
            return 0;
        }

        return world.get<Building>(entity)
            .stock[resourceIndex(resource) % kResourceCount];
    }

}
