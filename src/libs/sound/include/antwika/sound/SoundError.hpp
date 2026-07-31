#pragma once

#include <stdexcept>

namespace antwika::sound
{

    /**
     * @brief Thrown when a sound device or a decoder cannot honour a
     * request.
     *
     * Deliberately a single, specific, catchable type, mirroring
     * antwika::gfx::GfxError -- which is likewise what both its backends
     * and its PNG decoder raise.
     * Every backend raises this same type, so application code never
     * learns which audio framework it was built against from the
     * exceptions it catches.
     */
    class SoundError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

} // namespace antwika::sound
