#pragma once

#include <stdexcept>

namespace antwika::life
{

    /**
     * @brief Thrown by BoardSink when a life.toggle_cell payload is not
     * valid JSON, or not an object with unsigned integer "x" and "y"
     * fields that fit in a std::uint32_t.
     */
    class BoardSinkError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

} // namespace antwika::life
