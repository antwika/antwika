#pragma once

#include <antwika/time/Tick.hpp>

namespace antwika::input
{

    class IFramePump
    {
    public:
        virtual ~IFramePump() = default;

        virtual void pump(antwika::time::Tick tick) = 0;
    };

}
