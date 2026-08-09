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

        virtual FrameCount pump(FrameCount frames) = 0;

        [[nodiscard]] virtual WaveFormat format() const = 0;

        [[nodiscard]] virtual FrameCount bufferFrames() const = 0;

        [[nodiscard]] virtual FrameIndex framesPlayed() const = 0;
    };

}
