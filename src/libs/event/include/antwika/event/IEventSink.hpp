#pragma once

#include "antwika/event/Event.hpp"

namespace antwika::event
{

    /**
     * @brief Consumer of dispatched events.
     */
    class IEventSink
    {
    public:
        virtual ~IEventSink() = default;

        /**
         * @brief Handle a dispatched event.
         * @param event The event to handle.
         */
        virtual void handle(const Event &event) = 0;
    };

} // namespace antwika::event
