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
        void render(SampleBuffer out, FrameIndex firstFrame) noexcept
            override
        {
            for (std::size_t frame = 0; frame < out.frames; ++frame)
            {
                for (const auto channel : out.channels)
                {
                    channel[frame] =
                        static_cast<float>(firstFrame + frame);
                }
            }
        }
    };

}
