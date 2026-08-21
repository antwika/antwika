#pragma once

#include <cstddef>
#include <vector>

#include "antwika/sound/Frames.hpp"
#include "antwika/sound/IRenderCallback.hpp"
#include "antwika/sound/SampleBuffer.hpp"

namespace antwika::sound::fakes
{

    class FakeRecordingCallback final : public IRenderCallback
    {
    public:
        struct Call final
        {
            FrameIndex firstFrame = 0;
            FrameCount frames = 0;
            std::size_t channels = 0;
            bool complete = false;
        };

        void render(SampleBuffer samples, FrameIndex firstFrame) noexcept
            override
        {
            if (inside)
            {
                reentered = true;
            }

            inside = true;

            calls.push_back(
                Call{
                    .firstFrame = firstFrame,
                    .frames = samples.frames,
                    .channels = samples.channels.size(),
                    .complete = samples.isValid()});

            samples.silence();
            inside = false;
        }

        std::vector<Call> calls;
        bool reentered = false;

    private:
        bool inside = false;
    };

}
