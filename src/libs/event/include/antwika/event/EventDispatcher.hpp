#pragma once

#include <vector>

#include "antwika/event/IEventDispatcher.hpp"
#include "antwika/event/IEventSink.hpp"

namespace antwika::event
{

    class EventDispatcher final : public IEventDispatcher
    {
    public:
        explicit EventDispatcher(
            std::vector<std::reference_wrapper<IEventSink>> sinks);

        EventDispatcher(const EventDispatcher &) = delete;
        EventDispatcher(EventDispatcher &&) = delete;

        EventDispatcher &operator=(const EventDispatcher &) = delete;
        EventDispatcher &operator=(EventDispatcher &&) = delete;

        void dispatch(Event event) override;

    private:
        std::vector<std::reference_wrapper<IEventSink>> sinks;
    };

}
