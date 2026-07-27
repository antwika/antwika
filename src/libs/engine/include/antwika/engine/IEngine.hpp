#pragma once

#include <antwika/time/Tick.hpp>

namespace antwika::engine
{

    class IEngine
    {
    public:
        virtual ~IEngine() = default;
        virtual void start() = 0;
        virtual void step(antwika::time::Tick tick) = 0;
    };

} // namespace antwika::engine
