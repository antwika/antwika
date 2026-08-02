#pragma once

#include <stdexcept>

namespace antwika::event
{

    /**
     * @brief A misuse of the event mechanism itself.
     *
     * One exception type for the module's one failure category: using
     * the dispatch path in a way no recording could reproduce.  So
     * far that is exactly one case, re-entering dispatch() from
     * inside a sink.
     */
    class EventError : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

} // namespace antwika::event
