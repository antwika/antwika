#pragma once

#include <vector>

#include "Event.hpp"
#include "IEventHistory.hpp"
#include "IEventSink.hpp"

namespace antwika::event
{

    /**
     * @brief Sink that records every event it handles for later retrieval.
     */
    class EventRecorder final : public IEventSink,
                                public IEventHistory
    {
    public:
        /**
         * @brief Append an event to the recorded history.
         * @param event The event to record.
         */
        void handle(const Event &event) override;

        /**
         * @brief Get all events recorded so far.
         * @return The recorded events, in the order they were received.
         */
        [[nodiscard]] std::vector<Event> getEvents() const override;

    private:
        std::vector<Event> events;
    };

} // namespace antwika::event
