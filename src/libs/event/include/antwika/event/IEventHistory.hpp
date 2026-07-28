#pragma once

#include <vector>

#include "Event.hpp"

namespace antwika::event
{

    /**
     * @brief Provides read access to a recorded sequence of events.
     */
    class IEventHistory
    {
    public:
        virtual ~IEventHistory() = default;

        /**
         * @brief Get all events recorded so far.
         * @return The recorded events, in the order they were received.
         */
        [[nodiscard]] virtual std::vector<Event> getEvents() const = 0;
    };

} // namespace antwika::event
