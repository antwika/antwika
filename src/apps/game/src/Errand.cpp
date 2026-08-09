#include "antwika/game/Errand.hpp"

namespace antwika::game
{

    antwika::ecs::Entity errandTargetOf(
        const antwika::ecs::World &world, antwika::ecs::Entity entity)
    {
        if (!world.has<Errand>(entity))
        {
            return antwika::ecs::kNullEntity;
        }

        return errandTarget(
            world.get<Errand>(entity), world.get<Walker>(entity));
    }

}
