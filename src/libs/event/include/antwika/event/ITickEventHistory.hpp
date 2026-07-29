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
         * @return The recorded events, in the order they were received.
         */
        [[nodiscard]] virtual std::vector<TickEvent> getEvents() const = 0;
    };

} // namespace antwika::event
