#pragma once

#include <vector>

#include "antwika/event/ITickEventSink.hpp"
#include "antwika/event/TickEvent.hpp"

namespace antwika::event
{

    /**
     * @brief Sink that records every tick event it handles for later replay.
     */
    class TickEventRecorder final : public ITickEventSink
    {
    public:
        /**
         * @brief Append a tick event to the recorded history.
         * @param event The tick event to record.
         */
        void handle(const TickEvent &event) override;

        /**
         * @brief Get all tick events recorded so far.
         *
         * By reference, so that asking what has been recorded costs
         * nothing.
         * A recording is every event of a session, and saveReplayFile
         * already takes its vector by value, so the copy belongs there
         * rather than in every call.
         *
         * @return The recorded events, in the order they were received.
         * The vector outlives the call and grows as more arrive, so a
         * caller keeping it must keep the recorder too.
         */
        [[nodiscard]] const std::vector<TickEvent> &getEvents() const;

    private:
        std::vector<TickEvent> events;
    };

} // namespace antwika::event
