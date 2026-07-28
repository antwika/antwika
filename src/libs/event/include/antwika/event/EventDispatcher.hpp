#pragma once

#include <vector>

#include "IEventDispatcher.hpp"
#include "IEventSink.hpp"

namespace antwika::event
{

    /**
     * @brief IEventDispatcher that fans a dispatched event out to its sinks.
     */
    class EventDispatcher final : public IEventDispatcher
    {
    public:
        /**
         * @brief Construct a dispatcher over its sinks.
         * @param sinks Sinks that will receive every dispatched event.
         */
        explicit EventDispatcher(
            std::vector<std::reference_wrapper<IEventSink>> sinks);

        EventDispatcher(const EventDispatcher &) = delete;
        EventDispatcher(EventDispatcher &&) = delete;

        EventDispatcher &operator=(const EventDispatcher &) = delete;
        EventDispatcher &operator=(EventDispatcher &&) = delete;

        /**
         * @brief Deliver an event to all sinks.
         * @param event The event to dispatch.
         */
        void dispatch(Event event) override;

    private:
        std::vector<std::reference_wrapper<IEventSink>> sinks;
    };

} // namespace antwika::event
