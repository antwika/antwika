#pragma once

#include "antwika/sound/Frames.hpp"
#include "antwika/sound/WaveFormat.hpp"

namespace antwika::sound
{

    inline constexpr FrameCount kDefaultBufferFrames = 1024;

    struct DeviceDesc final
    {
        WaveFormat format;
        FrameCount preferredBufferFrames = kDefaultBufferFrames;

        [[nodiscard]] bool operator==(const DeviceDesc &other) const
            = default;
    };

}
