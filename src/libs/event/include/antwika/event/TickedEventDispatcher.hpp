#pragma once

#include <functional>
#include <vector>

#include <antwika/time/Tick.hpp>

#include "IEventDispatcher.hpp"
#include "ITimedEventSink.hpp"

namespace antwika::event
{

    class TickedEventDispatcher final : public IEventDispatcher
    {
    public:
        TickedEventDispatcher(IEventDispatcher &dispatcher,
                              std::vector<std::reference_wrapper<ITimedEventSink>> timedSinks);

        TickedEventDispatcher(const TickedEventDispatcher &) = delete;
        TickedEventDispatcher(TickedEventDispatcher &&) = delete;

        TickedEventDispatcher &operator=(const TickedEventDispatcher &) = delete;
        TickedEventDispatcher &operator=(TickedEventDispatcher &&) = delete;

        void setTick(antwika::time::Tick tick) noexcept;

        void dispatch(Event event) override;

    private:
        IEventDispatcher &dispatcher;
        std::vector<std::reference_wrapper<ITimedEventSink>> timedSinks;
        antwika::time::Tick currentTick{};
    };

} // namespace antwika::event
