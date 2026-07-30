#pragma once

#include <vector>

#include "antwika/event/Event.hpp"

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
        [[nodiscard]] virtual const std::vector<Event> &getEvents()
            const = 0;
    };

} // namespace antwika::event
