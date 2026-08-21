#include "antwika/sound/OfflineDevice.hpp"

#include "antwika/sound/SoundError.hpp"
#include "RenderInChunks.hpp"

namespace antwika::sound
{

    OfflineDevice::OfflineDevice(
        const DeviceSpec &spec, Waveform &destinationWaveform)
        : wave(spec.format),
          bufferCount(
              spec.preferredBufferFrames > 0 ? spec.preferredBufferFrames
                                             : kDefaultBufferFrames),
          destinationWaveform(destinationWaveform)
    {
        destinationWaveform.format = wave;
    }

    void OfflineDevice::start(IRenderCallback &sourceCallback)
    {
        if (callback != nullptr)
        {
            throw SoundError(
                "antwika::sound: the device is already started");
        }

        callback = &sourceCallback;
    }

    void OfflineDevice::stop()
    {
        callback = nullptr;
    }

    FrameCount OfflineDevice::advance(FrameCount frames)
    {
        if (callback == nullptr)
        {
            return 0;
        }

        const auto doneFrames = detail::renderInChunks(
            *callback,
            wave.channels,
            bufferCount,
            frames,
            playedIndex,
            [this](const detail::Planes &planes, FrameCount chunk)
            {
                for (FrameCount frame = 0; frame < chunk; ++frame)
                {
                    for (const auto &plane : planes)
                    {
                        destinationWaveform.samples.push_back(plane[frame]);
                    }
                }
            });

        playedIndex += doneFrames;
        return doneFrames;
    }

    WaveFormat OfflineDevice::format() const
    {
        return wave;
    }

    FrameCount OfflineDevice::bufferFrames() const
    {
        return bufferCount;
    }

    FrameIndex OfflineDevice::framesPlayed() const
    {
        return playedIndex;
    }

}
