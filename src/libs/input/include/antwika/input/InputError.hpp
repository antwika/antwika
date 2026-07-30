#pragma once

#include <stdexcept>

namespace antwika::input
{

    /**
     * @brief Thrown when input cannot be made sense of, such as a key name
     * no key goes by, or a backend that failed to start.
     *
     * Deliberately a single, specific, catchable type, mirroring
     * antwika::gfx::GfxError and antwika::replay::ReplayFormatError.
     * Every backend raises this same type, so application code never
     * learns which input framework it was built against from the
     * exceptions it catches.
     */
    class InputError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

} // namespace antwika::input
