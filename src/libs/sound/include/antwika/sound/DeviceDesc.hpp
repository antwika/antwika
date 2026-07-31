#pragma once

#include "antwika/sound/Frames.hpp"
#include "antwika/sound/WaveFormat.hpp"

namespace antwika::sound
{

    /** @brief How many frames a device asks for at a time by default. */
    inline constexpr FrameCount kDefaultBufferFrames = 1024;

    /**
     * @brief What a caller wants a device opened as.
     *
     * A request rather than a demand for the buffer size: a backend
     * reports what it actually got through IDevice::bufferFrames().
     * The format is not negotiable, and a backend that cannot honour it
     * refuses rather than quietly opening something else.
     */
    struct DeviceDesc
    {
        WaveFormat format;
        FrameCount preferredBufferFrames = kDefaultBufferFrames;

        /**
         * @brief Compare two descriptions.
         * @param other The description to compare against.
         * @return True when every field matches.
         */
        [[nodiscard]] bool operator==(const DeviceDesc &other) const
            = default;
    };

} // namespace antwika::sound
