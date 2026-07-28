#pragma once

#include <vector>

#include "IEventDispatcher.hpp"
#include "IEventQueue.hpp"
#include "IEventSink.hpp"

namespace antwika::event
{

    /**
     * @brief IEventDispatcher that enqueues events and fans them out to sinks.
     */
    class EventDispatcher final : public IEventDispatcher
    {
    public:
        /**
         * @brief Construct a dispatcher over a queue and its sinks.
         * @param queue Queue used to buffer events before delivery.
         * @param sinks Sinks that will receive every dispatched event.
         */
        EventDispatcher(IEventQueue &queue, std::vector<std::reference_wrapper<IEventSink>> sinks);

        EventDispatcher(const EventDispatcher &) = delete;
        EventDispatcher(EventDispatcher &&) = delete;

        EventDispatcher &operator=(const EventDispatcher &) = delete;
        EventDispatcher &operator=(EventDispatcher &&) = delete;

        /**
         * @brief Deliver an event to all sinks, then enqueue it.
         * @param event The event to dispatch.
         */
        void dispatch(Event event) override;

    private:
        IEventQueue &queue;
        std::vector<std::reference_wrapper<IEventSink>> sinks;
    };

} // namespace antwika::event
