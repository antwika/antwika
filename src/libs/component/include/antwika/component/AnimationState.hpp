#pragma once

#include <cstdint>

#include <antwika/time/Tick.hpp>

namespace antwika::component
{

    struct AnimationState final
    {
        std::uint8_t direction = 0;

        bool walking = false;

        time::Tick startedAtTick = 0;
    };

}
