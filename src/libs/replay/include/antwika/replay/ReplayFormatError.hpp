#pragma once

#include <stdexcept>

namespace antwika::replay
{

    // Thrown when a replay stream is malformed.
    // That includes bad magic bytes.
    // It also includes an unsupported format version.
    // It also includes a stream that ends before the data it claims to hold.
    // Deliberately a single, specific, catchable type.
    // It avoids surfacing truncation/corruption as unspecific std::runtime_error.
    // It also avoids surfacing it as undefined behavior.
    class ReplayFormatError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

} // namespace antwika::replay
