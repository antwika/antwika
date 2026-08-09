#pragma once

#include <cstddef>

#include "antwika/sound/Frames.hpp"
#include "antwika/sound/IRenderCallback.hpp"
#include "antwika/sound/SampleBuffer.hpp"

namespace antwika::sound::fakes
{

    class FakePerChannelCallback final : public IRenderCallback
    {
    public:
        void render(SampleBuffer out, FrameIndex firstFrame) noexcept
            override
        {
            for (std::size_t frame = 0; frame < out.frames; ++frame)
            {
                std::size_t channel = 0;
                for (const auto samples : out.channels)
                {
                    samples[frame] = static_cast<float>(
                        firstFrame + frame + (100 * channel));
                    ++channel;
                }
            }
        }
    };

}
