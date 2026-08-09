#pragma once

#include <functional>
#include <vector>

#include <antwika/time/Tick.hpp>

#include "antwika/event/IEventDispatcher.hpp"
#include "antwika/event/ITickEventSink.hpp"

namespace antwika::event
{

    class TickedEventDispatcher final : public IEventDispatcher
    {
    public:
        TickedEventDispatcher(
            IEventDispatcher &dispatcher,
            std::vector<std::reference_wrapper<ITickEventSink>> timedSinks);

        TickedEventDispatcher(const TickedEventDispatcher &) = delete;
        TickedEventDispatcher(TickedEventDispatcher &&) = delete;

        TickedEventDispatcher &operator=(
            const TickedEventDispatcher &) = delete;
        TickedEventDispatcher &operator=(TickedEventDispatcher &&) = delete;

        void setTick(antwika::time::Tick tick) noexcept;

        void dispatch(Event event) override;

    private:
        IEventDispatcher &dispatcher;
        std::vector<std::reference_wrapper<ITickEventSink>> timedSinks;
        antwika::time::Tick currentTick{};

        bool dispatching = false;
    };

}
