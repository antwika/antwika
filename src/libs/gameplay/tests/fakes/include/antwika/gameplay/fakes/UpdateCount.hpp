#pragma once

#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/time/Tick.hpp>

namespace antwika::gameplay::fakes
{

    struct UpdateCount final
    {
        int seenCount = 0;
    };

}
