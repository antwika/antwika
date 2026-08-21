#pragma once

#include "antwika/sound/Frames.hpp"
#include "antwika/sound/WaveformId.hpp"

namespace antwika::sound
{

    struct PlayRequest final
    {
        WaveformId waveform{};

        FrameIndex startFrame = 0;

        float gain = 1.0F;

        float pan = 0.0F;

        bool looping = false;

        [[nodiscard]] bool operator==(const PlayRequest &other) const
            = default;
    };

}
