#pragma once

#include <deque>

#include "Event.hpp"
#include "IEventSink.hpp"
#include "IEventQueue.hpp"

namespace antwika::event
{

    /**
     * @brief IEventQueue implementation backed by a std::deque.
     */
    class EventQueue final : public IEventQueue
    {
    public:
        /**
         * @brief Add an event to the back of the queue.
         * @param event The event to enqueue.
         */
        void enqueue(Event event) override;

        /**
         * @brief Remove and return the event at the front of the queue.
         * @return The removed event.
         */
        [[nodiscard]] Event pop() override;

        /**
         * @brief Check whether the queue holds no events.
         * @return true if the queue is empty, false otherwise.
         */
        [[nodiscard]] bool empty() const noexcept override;

    private:
        std::deque<Event> queue;
    };

} // namespace antwika::event
