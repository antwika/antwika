#pragma once

#include <antwika/event/ITimedEventSink.hpp>
#include <antwika/event/TimedEvent.hpp>

namespace antwika::engine
{

    using antwika::event::ITimedEventSink;
    using antwika::event::TimedEvent;

    /**
     * @brief Observes engine.stop and remembers that it was dispatched.
     *
     * Registered as a TickedEventDispatcher timed sink the same way an
     * application's own reducers are -- it has no special status in the
     * event pipeline. EngineLoop holds a reference to one and checks
     * stopped() after every tick to decide whether to keep running.
     */
    class StopSignal final : public ITimedEventSink
    {
    public:
        /**
         * @brief Record that a stop was requested, if this event is one.
         * @param event The timed event to inspect.
         */
        void handle(const TimedEvent &event) override;

        /**
         * @brief Whether engine.stop has been observed yet.
         * @return True once engine.stop has been handled, false until then.
         */
        [[nodiscard]] bool stopped() const noexcept;

    private:
        bool stopped_ = false;
    };

} // namespace antwika::engine
