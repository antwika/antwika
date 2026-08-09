#pragma once

#include <antwika/sound/Frames.hpp>

#include "antwika/synth/VoiceDesc.hpp"

namespace antwika::synth
{

    using antwika::sound::FrameIndex;

    struct TriggerRequest final
    {
        VoiceDesc voice{};

        FrameIndex startFrame = 0;

        [[nodiscard]] bool operator==(const TriggerRequest &other) const
            = default;
    };

}
