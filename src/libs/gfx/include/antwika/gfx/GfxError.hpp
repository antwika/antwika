#pragma once

#include <stdexcept>

namespace antwika::gfx
{

    /**
     * @brief Thrown when a graphics backend cannot honour a request, such
     * as a window that could not be created.
     *
     * Deliberately a single, specific, catchable type, mirroring
     * antwika::ecs::EcsError and antwika::replay::ReplayFormatError.
     * Every backend raises this same type, so application code never
     * learns which graphics framework it was built against from the
     * exceptions it catches.
     */
    class GfxError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

} // namespace antwika::gfx
