#pragma once

#include "antwika/sound/Frames.hpp"
#include "antwika/sound/IRenderCallback.hpp"
#include "antwika/sound/WaveFormat.hpp"

namespace antwika::sound
{

    class IDevice
    {
    public:
        virtual ~IDevice() = default;

        virtual void start(IRenderCallback &callback) = 0;

        virtual void stop() = 0;

        virtual FrameCount advance(FrameCount frames) = 0;

        [[nodiscard]] virtual WaveFormat getFormat() const = 0;

        [[nodiscard]] virtual FrameCount getBufferFrames() const = 0;

        [[nodiscard]] virtual FrameIndex getFramesPlayed() const = 0;
    };

}
