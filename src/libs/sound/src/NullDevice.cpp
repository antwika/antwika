#include "antwika/sound/NullDevice.hpp"

#include "antwika/sound/SoundError.hpp"
#include "RenderInChunks.hpp"

namespace antwika::sound
{

    NullDevice::NullDevice(const DeviceDesc &desc)
        : wave(desc.format),
          buffer(
              desc.preferredBufferFrames > 0 ? desc.preferredBufferFrames
                                             : kDefaultBufferFrames)
    {
    }

    void NullDevice::start(IRenderCallback &callback)
    {
        if (sink != nullptr)
        {
            throw SoundError(
                "antwika::sound: the device is already started");
        }

        sink = &callback;
    }

    void NullDevice::stop()
    {
        sink = nullptr;
    }

    FrameCount NullDevice::pump(FrameCount frames)
    {
        if (sink == nullptr)
        {
            return 0;
        }

        // A rendered chunk goes nowhere, which is the whole device.
        const auto done = detail::renderInChunks(
            *sink,
            wave.channels,
            buffer,
            frames,
            played,
            [](const detail::Planes &, FrameCount) {});

        played += done;
        return done;
    }

    WaveFormat NullDevice::format() const
    {
        return wave;
    }

    FrameCount NullDevice::bufferFrames() const
    {
        return buffer;
    }

    FrameIndex NullDevice::framesPlayed() const
    {
        return played;
    }

} // namespace antwika::sound
