#pragma once

#include "antwika/event/TickEvent.hpp"

namespace antwika::event
{

    /**
     * @brief Consumer of dispatched tick events.
     */
    class ITickEventSink
    {
    public:
        virtual ~ITickEventSink() = default;

        /**
         * @brief Handle a dispatched tick event.
         * @param event The tick event to handle.
         */
        virtual void handle(const TickEvent &event) = 0;
    };

} // namespace antwika::event
