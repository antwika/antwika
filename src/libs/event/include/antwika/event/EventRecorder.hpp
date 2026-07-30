#pragma once

#include <vector>

#include "antwika/event/Event.hpp"
#include "antwika/event/IEventSink.hpp"

namespace antwika::event
{

    /**
     * @brief Sink that records every event it handles for later retrieval.
     *
     * Nothing holds one of these through a history interface, so there
     * is no IEventHistory for it to implement.
     * The only recording anything reads back is the tick-stamped one a
     * --record run saves, and that is TickEventRecorder's.
     */
    class EventRecorder final : public IEventSink
    {
    public:
        /**
         * @brief Append an event to the recorded history.
         * @param event The event to record.
         */
        void handle(const Event &event) override;

        /**
         * @brief Get all events recorded so far.
         *
         * By reference, so that asking what has been recorded costs
         * nothing.
         * A caller that wants a copy takes one, at the one place that
         * needs it.
         *
         * @return The recorded events, in the order they were received.
         * The vector outlives the call and grows as more arrive, so a
         * caller keeping it must keep the recorder too.
         */
        [[nodiscard]] const std::vector<Event> &getEvents() const;

    private:
        std::vector<Event> events;
    };

} // namespace antwika::event
