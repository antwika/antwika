#pragma once

#include <stdexcept>

namespace antwika::replay
{

    /**
     * @brief Thrown when a replay document is not one this library can
     * read.
     *
     * That covers text that is not JSON at all -- which is what a
     * truncated file looks like -- a "magic" or "version" member that is
     * not this format's, a missing member, an unexpected one, and a tick
     * or an event of the wrong shape. The reader validates the whole
     * document against a schema, so all of them arrive here rather than
     * some of them surfacing later as a surprising replay.
     *
     * Deliberately a single, specific, catchable type, so that a
     * malformed replay is not an unspecific std::runtime_error from a
     * JSON parser two libraries away.
     */
    class ReplayFormatError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

} // namespace antwika::replay
