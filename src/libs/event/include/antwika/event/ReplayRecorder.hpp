#pragma once

#include <vector>

#include "antwika/event/ITimedEventHistory.hpp"
#include "antwika/event/ITimedEventSink.hpp"
#include "antwika/event/TimedEvent.hpp"

namespace antwika::event
{

    /**
     * @brief Sink that records every timed event it handles for later replay.
     */
    class ReplayRecorder final : public ITimedEventSink,
                                 public ITimedEventHistory
    {
    public:
        /**
         * @brief Append a timed event to the recorded history.
         * @param event The timed event to record.
         */
        void handle(const TimedEvent &event) override;

        /**
         * @brief Get all timed events recorded so far.
         * @return The recorded events, in the order they were received.
         */
        [[nodiscard]] std::vector<TimedEvent> getEvents() const override;

    private:
        std::vector<TimedEvent> events;
    };

} // namespace antwika::event
