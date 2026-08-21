#pragma once

#include "antwika/sound/Frames.hpp"
#include "antwika/sound/SampleBuffer.hpp"

namespace antwika::sound
{

    class IRenderCallback
    {
    public:
        virtual ~IRenderCallback() = default;

        virtual void render(
            SampleBuffer samples, FrameIndex firstFrame) noexcept = 0;
    };

}
