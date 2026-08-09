#pragma once

#include <antwika/pattern/Controls.hpp>
#include <antwika/sound/Frames.hpp>

namespace antwika::sequencer
{

    using antwika::pattern::Controls;
    using antwika::sound::FrameCount;
    using antwika::sound::FrameIndex;

    class ISequencerSink
    {
    public:
        virtual ~ISequencerSink() = default;

        virtual void trigger(
            const Controls &value,
            FrameIndex startFrame,
            FrameCount frames)
            = 0;
    };

}
