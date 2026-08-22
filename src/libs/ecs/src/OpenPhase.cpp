#include "antwika/ecs/OpenPhase.hpp"

namespace antwika::ecs
{

    OpenPhase::OpenPhase(World &world) noexcept : world(&world)
    {
    }

    OpenPhase::~OpenPhase()
    {
        world->commit();
    }

}
