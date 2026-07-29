#pragma once

#include "antwika/event/TimedEvent.hpp"

namespace antwika::event
{

    /**
     * @brief Consumer of dispatched timed events.
     */
    class ITimedEventSink
    {
    public:
        virtual ~ITimedEventSink() = default;

        /**
         * @brief Handle a dispatched timed event.
         * @param event The timed event to handle.
         */
        virtual void handle(const TimedEvent &event) = 0;
    };

} // namespace antwika::event
