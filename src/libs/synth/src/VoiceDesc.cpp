#include "antwika/synth/VoiceDesc.hpp"

#include <antwika/sound/Frames.hpp>

namespace antwika::synth
{

    FrameCount VoiceDesc::totalFrames() const noexcept
    {
        return hold + envelope.release;
    }

}
