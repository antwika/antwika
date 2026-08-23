#include "antwika/sound/NullDevice.hpp"

#include "antwika/sound/SoundError.hpp"
#include "RenderInChunks.hpp"

namespace antwika::sound
{

    NullDevice::NullDevice(const DeviceSpec &spec)
        : wave(spec.format),
          bufferCount(
              spec.preferredBufferFrames > 0 ? spec.preferredBufferFrames
                                             : kDefaultBufferFrames)
    {
    }

    void NullDevice::start(IRenderCallback &callback)
    {
        if (sinkCallback != nullptr)
        {
            throw SoundError(
                "antwika::sound: the device is already started");
        }

        sinkCallback = &callback;
    }

    void NullDevice::stop()
    {
        sinkCallback = nullptr;
    }

    FrameCount NullDevice::advance(FrameCount frames)
    {
        if (sinkCallback == nullptr)
        {
            return 0;
        }

        const auto doneFrames = detail::renderInChunks(
            *sinkCallback,
            wave.channels,
            bufferCount,
            frames,
            playedIndex,
            [](const detail::Planes &, FrameCount) {});

        playedIndex += doneFrames;
        return doneFrames;
    }

    WaveFormat NullDevice::getFormat() const
    {
        return wave;
    }

    FrameCount NullDevice::getBufferFrames() const
    {
        return bufferCount;
    }

    FrameIndex NullDevice::getFramesPlayed() const
    {
        return playedIndex;
    }

}
