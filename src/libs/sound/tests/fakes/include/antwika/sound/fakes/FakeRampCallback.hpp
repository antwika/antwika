#pragma once

#include <cstddef>

#include "antwika/sound/Frames.hpp"
#include "antwika/sound/IRenderCallback.hpp"
#include "antwika/sound/SampleBuffer.hpp"

namespace antwika::sound::fakes
{

    class FakeRampCallback final : public IRenderCallback
    {
    public:
        void render(SampleBuffer samples, FrameIndex firstFrame) noexcept
            override
        {
            for (std::size_t frame = 0; frame < samples.frames; ++frame)
            {
                for (const auto channel : samples.channels)
                {
                    channel[frame] =
                        static_cast<float>(firstFrame + frame);
                }
            }
        }
    };

}
