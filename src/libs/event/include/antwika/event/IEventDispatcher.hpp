#pragma once

#include "Event.hpp"

namespace antwika::event
{

    class IEventDispatcher
    {
    public:
        virtual ~IEventDispatcher() = default;
        virtual void dispatch(Event event) = 0;
    };

} // namespace antwika::event
