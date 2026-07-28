#pragma once

#include <string>

namespace antwika::event
{
    /**
     * @brief A named piece of data flowing through the event system.
     */
    struct Event
    {
        std::string name{};    ///< Identifier used to distinguish event kinds.
        std::string payload{}; ///< Opaque, event-specific data.
        bool operator==(const Event &other) const = default;
    };

} // namespace antwika::event
