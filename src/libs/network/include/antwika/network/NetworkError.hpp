#pragma once

#include <stdexcept>

namespace antwika::network
{

    /**
     * @brief Thrown when a host cannot honour a request.
     *
     * Deliberately a single, specific, catchable type, mirroring
     * antwika::sound::SoundError and antwika::gfx::GfxError.
     * Every backend raises this same type, so application code never
     * learns which transport it was built against from the exceptions
     * it catches.
     *
     * **A peer going away is not one of these.**
     * A link dropping is what a network does rather than a request
     * being refused, so it is reported by the peer leaving peers() and
     * never by unwinding out of a tick.
     */
    class NetworkError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

} // namespace antwika::network
