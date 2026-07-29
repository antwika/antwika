#pragma once

#include "antwika/event/Event.hpp"

namespace antwika::event
{

    /**
     * @brief Sends events to whatever consumes them.
     */
    class IEventDispatcher
    {
    public:
        virtual ~IEventDispatcher() = default;

        /**
         * @brief Dispatch an event to its consumers.
         * @param event The event to dispatch.
         */
        virtual void dispatch(Event event) = 0;
    };

} // namespace antwika::event
