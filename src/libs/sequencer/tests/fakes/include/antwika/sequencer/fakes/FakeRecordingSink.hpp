#pragma once

#include <vector>

#include <antwika/pattern/Controls.hpp>
#include <antwika/sound/Frames.hpp>

#include "antwika/sequencer/ISequencerSink.hpp"

namespace antwika::sequencer::fakes
{

    struct Sounded final
    {
        pattern::Controls value;
        sound::FrameIndex startFrame = 0;
        sound::FrameCount frames = 0;
    };

    class FakeRecordingSink final : public ISequencerSink
    {
    public:
        void trigger(
            const pattern::Controls &value,
            sound::FrameIndex startFrame,
            sound::FrameCount frames) override
        {
            triggers.push_back(
                Sounded{
                    .value = value,
                    .startFrame = startFrame,
                    .frames = frames});
        }

        std::vector<Sounded> triggers;
    };

}
