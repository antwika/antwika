#include "antwika/synth/VoiceDesc.hpp"

#include <antwika/sound/Frames.hpp>

namespace antwika::synth
{

    FrameCount VoiceDesc::totalFrames() const noexcept
    {
        // The attack and decay sit inside the hold, not after it.
        // That lets a short effect be cut off while still rising.
        // See envelopeAt().
        return hold + envelope.release;
    }

} // namespace antwika::synth
