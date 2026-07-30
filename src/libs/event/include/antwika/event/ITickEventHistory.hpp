#pragma once

#include <vector>

#include "antwika/event/TickEvent.hpp"

namespace antwika::event
{

    /**
     * @brief Provides read access to a recorded sequence of tick events.
     */
    class ITickEventHistory
    {
    public:
        virtual ~ITickEventHistory() = default;

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
        [[nodiscard]] virtual const std::vector<TickEvent> &getEvents()
            const = 0;
    };

} // namespace antwika::event
