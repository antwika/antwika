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
        void dispatch(Event event) override;

    private:
        IEventQueue &queue;
        std::vector<std::reference_wrapper<IEventSink>> sinks;
    };

} // namespace antwika::event
