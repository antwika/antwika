#pragma once

#include <vector>

#include "IEventDispatcher.hpp"
#include "IEventQueue.hpp"
#include "IEventSink.hpp"

namespace antwika::event
{

    class EventDispatcher final : public IEventDispatcher
    {
    public:
        EventDispatcher(IEventQueue &queue, std::vector<std::reference_wrapper<IEventSink>> sinks);

        EventDispatcher(const EventDispatcher &) = delete;
        EventDispatcher(EventDispatcher &&) = delete;

        EventDispatcher &operator=(const EventDispatcher &) = delete;
        EventDispatcher &operator=(EventDispatcher &&) = delete;

        void dispatch(Event event) override;

    private:
        IEventQueue &queue;
        std::vector<std::reference_wrapper<IEventSink>> sinks;
    };

} // namespace antwika::event
