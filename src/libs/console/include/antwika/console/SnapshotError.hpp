#pragma once

#include <stdexcept>

namespace antwika::console
{

    /**
     * @brief A state dump that cannot be written, read or applied.
     *
     * The one failure category this library owns.
     * An application's store wraps its own errors into this at the
     * seam -- see ISnapshotStore -- so SnapshotCommands can turn a
     * failed load into a history line without naming any app's types.
     */
    class SnapshotError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

} // namespace antwika::console
