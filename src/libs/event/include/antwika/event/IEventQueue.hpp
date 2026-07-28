#pragma once

#include <vector>

#include "Event.hpp"

namespace antwika::event
{

    /**
     * @brief FIFO buffer of events awaiting dispatch.
     */
    class IEventQueue
    {
    public:
        virtual ~IEventQueue() = default;

        /**
         * @brief Add an event to the back of the queue.
         * @param event The event to enqueue.
         */
        virtual void enqueue(Event event) = 0;

        /**
         * @brief Remove and return the event at the front of the queue.
         * @return The removed event.
         */
        [[nodiscard]] virtual Event pop() = 0;

        /**
         * @brief Check whether the queue holds no events.
         * @return true if the queue is empty, false otherwise.
         */
        [[nodiscard]] virtual bool empty() const noexcept = 0;
    };

} // namespace antwika::event
