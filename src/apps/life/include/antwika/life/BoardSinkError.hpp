#pragma once

#include <stdexcept>

namespace antwika::life
{

    /**
     * @brief Thrown by BoardSink when a life.toggle_cell payload is
     * malformed: missing its comma separator, or either field is not a
     * plain, in-range unsigned integer.
     */
    class BoardSinkError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

} // namespace antwika::life
