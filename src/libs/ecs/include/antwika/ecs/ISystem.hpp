#pragma once

#include <antwika/time/Tick.hpp>

#include "antwika/ecs/World.hpp"

namespace antwika::ecs
{

    using antwika::time::Tick;

    class ISystem
    {
    public:
        virtual ~ISystem() = default;

        virtual void update(World &world, Tick tick) = 0;
    };

}
