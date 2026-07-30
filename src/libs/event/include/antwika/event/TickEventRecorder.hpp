#pragma once

#include <vector>

#include "antwika/event/ITickEventHistory.hpp"
#include "antwika/event/ITickEventSink.hpp"
#include "antwika/event/TickEvent.hpp"

namespace antwika::event
{

    /**
     * @brief Sink that records every tick event it handles for later replay.
     */
    class TickEventRecorder final : public ITickEventSink,
                                     public ITickEventHistory
    {
    public:
        /**
         * @brief Append a tick event to the recorded history.
         * @param event The tick event to record.
         */
        void handle(const TickEvent &event) override;

        /**
         * @brief Get all tick events recorded so far.
         * @return The recorded events, in the order they were received.
         */
        [[nodiscard]] const std::vector<TickEvent> &getEvents()
            const override;

    private:
        std::vector<TickEvent> events;
    };

} // namespace antwika::event
