#pragma once

#include <vector>

#include "antwika/event/TimedEvent.hpp"

namespace antwika::event
{

    /**
     * @brief Provides read access to a recorded sequence of timed events.
     */
    class ITimedEventHistory
    {
    public:
        virtual ~ITimedEventHistory() = default;

        /**
         * @brief Get all timed events recorded so far.
         * @return The recorded events, in the order they were received.
         */
        [[nodiscard]] virtual std::vector<TimedEvent> getEvents() const = 0;
    };

} // namespace antwika::event
