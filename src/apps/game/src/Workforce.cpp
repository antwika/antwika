#include "antwika/game/Workforce.hpp"

#include <antwika/ecs/Entity.hpp>

namespace antwika::game
{

    void setWorkforce(
        World &world, antwika::ecs::Entity entity, Workforce workforce)
    {
        if (world.has<Workforce>(entity))
        {
            world.set<Workforce>(entity, workforce);
            return;
        }

        world.add<Workforce>(entity, workforce);
    }

} // namespace antwika::game
