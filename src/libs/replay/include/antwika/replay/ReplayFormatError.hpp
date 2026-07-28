#pragma once

#include <stdexcept>

namespace antwika::replay
{

    /**
     * @brief Thrown when a replay stream is malformed: bad magic bytes, an
     * unsupported format version, or a stream that ends before the data it
     * claims to hold.
     *
     * Deliberately a single, specific, catchable type, to avoid surfacing
     * truncation/corruption as an unspecific std::runtime_error or as
     * undefined behavior.
     */
    class ReplayFormatError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

} // namespace antwika::replay
