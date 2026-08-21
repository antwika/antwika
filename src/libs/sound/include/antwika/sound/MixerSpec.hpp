#pragma once

#include <cstddef>
#include "antwika/sound/Frames.hpp"
#include "antwika/sound/IRenderCallback.hpp"
#include "antwika/sound/PlayRequest.hpp"
#include "antwika/sound/SampleBuffer.hpp"
#include "antwika/sound/WaveFormat.hpp"
#include "antwika/sound/WaveformLibrary.hpp"

namespace antwika::sound
{

    inline constexpr std::size_t kDefaultVoices = 32;

    struct MixerSpec final
    {
        WaveFormat format;
        std::size_t maxVoices = kDefaultVoices;
    };

}
