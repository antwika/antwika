#pragma once

#include <antwika/log/Logger.hpp>

#include "antwika/event/IEventQueue.hpp"

namespace antwika::engine
{

    class IEngine
    {
    public:
        virtual ~IEngine() = default;
        virtual void start() = 0;
    };

} // namespace antwika::engine
