#pragma once

#include <functional>
#include <vector>

#include <antwika/time/Tick.hpp>

#include "antwika/event/IEventDispatcher.hpp"
#include "antwika/event/ITimedEventSink.hpp"

namespace antwika::event
{

    /**
     * @brief IEventDispatcher decorator that stamps events with the current
     * tick and forwards them to timed sinks, in addition to an inner
     * dispatcher.
     */
    class TickedEventDispatcher final : public IEventDispatcher
    {
    public:
        /**
         * @brief Construct a dispatcher wrapping an inner dispatcher.
         * @param dispatcher Inner dispatcher that receives every event first.
         * @param timedSinks Sinks that receive events stamped with the
         * current tick.
         */
        TickedEventDispatcher(
            IEventDispatcher &dispatcher,
            std::vector<std::reference_wrapper<ITimedEventSink>> timedSinks);

        TickedEventDispatcher(const TickedEventDispatcher &) = delete;
        TickedEventDispatcher(TickedEventDispatcher &&) = delete;

        TickedEventDispatcher &operator=(
            const TickedEventDispatcher &) = delete;
        TickedEventDispatcher &operator=(TickedEventDispatcher &&) = delete;

        /**
         * @brief Set the tick value attached to subsequently dispatched events.
         * @param tick The current simulation tick.
         */
        void setTick(antwika::time::Tick tick) noexcept;

        /**
         * @brief Forward an event to the inner dispatcher and timed sinks.
         * @param event The event to dispatch.
         */
        void dispatch(Event event) override;

    private:
        IEventDispatcher &dispatcher;
        std::vector<std::reference_wrapper<ITimedEventSink>> timedSinks;
        antwika::time::Tick currentTick{};
    };

} // namespace antwika::event
